// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

#ifdef WITH_HUNSPELL
#include "spellchecker_hunspell.h"

#include "options.h"

#include <libaegisub/charset_conv.h>
#include <libaegisub/format.h>
#include <libaegisub/fs.h>
#include <libaegisub/io.h>
#include <libaegisub/line_iterator.h>
#include <libaegisub/log.h>
#include <libaegisub/path.h>

#include <boost/range/algorithm.hpp>

#include <algorithm>
#include <optional>

#include <unicode/uscript.h>
#include <unicode/utf8.h>

#undef near
#include <hunspell.hxx>

namespace {
/// The writing system of a word: that of its first letter
std::optional<int> WordScript(std::string_view word) {
	int32_t i = 0;
	int32_t length = static_cast<int32_t>(word.size());
	while (i < length) {
		UChar32 c;
		U8_NEXT(word.data(), i, length, c);
		if (c < 0) continue;
		UErrorCode err = U_ZERO_ERROR;
		UScriptCode script = uscript_getScript(c, &err);
		if (U_FAILURE(err) || script == USCRIPT_COMMON || script == USCRIPT_INHERITED || script == USCRIPT_UNKNOWN)
			continue;
		return script;
	}
	return std::nullopt;
}

/// Writing systems used by a language, e.g. Hebrew for he_IL
std::vector<int> LanguageScripts(std::string const& language) {
	UScriptCode codes[8];
	UErrorCode err = U_ZERO_ERROR;
	int count = uscript_getCode(language.c_str(), codes, 8, &err);
	if (U_FAILURE(err)) return {};
	return std::vector<int>(codes, codes + count);
}
}

HunspellSpellChecker::Dictionary::~Dictionary() = default;

bool HunspellSpellChecker::Dictionary::Covers(int script) const {
	return scripts.empty() || std::find(scripts.begin(), scripts.end(), script) != scripts.end();
}

HunspellSpellChecker::HunspellSpellChecker()
: lang_listener(OPT_SUB("Tool/Spell Checker/Language", &HunspellSpellChecker::OnLanguageChanged, this))
, dict_path_listener(OPT_SUB("Path/Dictionary", &HunspellSpellChecker::OnPathChanged, this))
{
	OnLanguageChanged();
}

HunspellSpellChecker::~HunspellSpellChecker() = default;

HunspellSpellChecker::Dictionary *HunspellSpellChecker::DictionaryFor(std::string_view word) {
	if (!enabled) return nullptr;

	auto script = WordScript(word);
	if (!script || (primary && primary->Covers(*script)))
		return primary.get();

	// The word is in a writing system the chosen dictionary can't check, such
	// as Hebrew with an English dictionary. Use an installed dictionary for
	// that writing system if there is one, preferring the interface language.
	auto it = by_script.find(*script);
	if (it == by_script.end()) {
		std::string ui_language = OPT_GET("App/Language")->GetString();
		std::string found;
		for (auto const& language : GetLanguageList()) {
			if (primary && language == primary->language) continue;
			auto scripts = LanguageScripts(language);
			if (std::find(scripts.begin(), scripts.end(), *script) == scripts.end()) continue;
			if (found.empty() || (!ui_language.empty() && language.starts_with(ui_language.substr(0, 2))))
				found = language;
		}
		it = by_script.emplace(*script, found.empty() ? nullptr : LoadDictionary(found)).first;
	}
	return it->second.get();
}

bool HunspellSpellChecker::CanAddWord(std::string_view word) {
	auto dict = DictionaryFor(word);
	if (!dict) return false;
	try {
		dict->conv->Convert(word);
		return true;
	}
	catch (agi::charset::ConvError const&) {
		return false;
	}
}

bool HunspellSpellChecker::CanRemoveWord(std::string_view word) {
	auto dict = DictionaryFor(word);
	return dict && dict->customWords.count(word);
}

void HunspellSpellChecker::AddWord(std::string_view word) {
	auto dict = DictionaryFor(word);
	if (!dict) return;

	// Add it to the in-memory dictionary
	dict->hunspell->add(dict->conv->Convert(word));

	// Add the word
	if (dict->customWords.insert(std::string(word)).second)
		SaveUserWords(*dict);
}

void HunspellSpellChecker::RemoveWord(std::string_view word) {
	auto dict = DictionaryFor(word);
	if (!dict) return;

	// Remove it from the in-memory dictionary
	dict->hunspell->remove(dict->conv->Convert(word));

	auto word_iter = dict->customWords.find(word);
	if (word_iter != dict->customWords.end()) {
		dict->customWords.erase(word_iter);
		SaveUserWords(*dict);
	}
}

void HunspellSpellChecker::Dictionary::ReadUserDictionary() {
	customWords.clear();

	// Read the old contents of the user's dictionary
	try {
		auto stream = agi::io::Open(userDicPath);
		copy_if(
			++agi::line_iterator<std::string>(*stream), agi::line_iterator<std::string>(),
			inserter(customWords, customWords.end()),
			[](auto const& str) { return !str.empty(); });
	}
	catch (agi::fs::FileNotFound const&) {
		// Not an error; user dictionary just doesn't exist
	}
}

void HunspellSpellChecker::Dictionary::WriteUserDictionary() const {
	// Ensure that the path exists
	agi::fs::CreateDirectory(userDicPath.parent_path());

	// Write the new dictionary
	agi::io::Save writer(userDicPath);
	writer.Get() << customWords.size() << "\n";
	copy(customWords.begin(), customWords.end(), std::ostream_iterator<std::string>(writer.Get(), "\n"));
}

void HunspellSpellChecker::SaveUserWords(Dictionary const& dict) {
	dict.WriteUserDictionary();

	// Announce a language change so that any other spellcheckers reload the
	// dictionaries to get the addition/removal
	lang_listener.Block();
	OPT_SET("Tool/Spell Checker/Language")->SetString(OPT_GET("Tool/Spell Checker/Language")->GetString());
	lang_listener.Unblock();
}

bool HunspellSpellChecker::CheckWord(std::string_view word) {
	auto dict = DictionaryFor(word);
	// Don't flag words that no installed dictionary can check
	if (!dict) return true;
	try {
		return dict->hunspell->spell(dict->conv->Convert(word));
	}
	catch (agi::charset::ConvError const&) {
		return false;
	}
}

std::vector<std::string> HunspellSpellChecker::GetSuggestions(std::string_view word) {
	std::vector<std::string> suggestions;
	auto dict = DictionaryFor(word);
	if (!dict) return suggestions;

	std::vector<std::string> results;
	try {
		results = dict->hunspell->suggest(dict->conv->Convert(word));
	}
	catch (agi::charset::ConvError const&) {
		return suggestions;
	}

	// Convert suggestions to UTF-8
	for (auto const& result : results) {
		try {
			suggestions.push_back(dict->rconv->Convert(result));
		}
		catch (agi::charset::ConvError const&) {
			// Shouldn't ever actually happen...
		}
	}

	return suggestions;
}

static std::vector<std::string> langs(const char *filter) {
	std::vector<std::string> paths;
	auto data_path = config::path->Decode("?dictionary/");
	auto user_path = config::path->Decode(OPT_GET("Path/Dictionary")->GetString());

	agi::fs::DirectoryIterator(data_path, filter).GetAll(paths);
	agi::fs::DirectoryIterator(user_path, filter).GetAll(paths);

	// Drop extensions
	for (auto& fn : paths) fn.resize(fn.size() - 4);

	boost::sort(paths);
	paths.erase(unique(begin(paths), end(paths)), end(paths));

	return paths;
}

std::vector<std::string> HunspellSpellChecker::GetLanguageList() {
	if (languages.empty())
		boost::set_intersection(langs("*.dic"), langs("*.aff"), back_inserter(languages));
	return languages;
}

static bool check_path(agi::fs::path const& path, std::string_view language,
	                   agi::fs::path& aff, agi::fs::path& dic) {
	aff = path/agi::format("%s.aff", language);
	dic = path/agi::format("%s.dic", language);
	return agi::fs::FileExists(aff) && agi::fs::FileExists(dic);
}

std::unique_ptr<HunspellSpellChecker::Dictionary> HunspellSpellChecker::LoadDictionary(std::string const& language) {
	agi::fs::path aff, dic;
	auto path = config::path->Decode(OPT_GET("Path/Dictionary")->GetString() + "/");
	if (!check_path(path, language, aff, dic)) {
		path = config::path->Decode("?dictionary/");
		if (!check_path(path, language, aff, dic))
			return nullptr;
	}

	LOG_I("dictionary/file") << dic;

	auto dict = std::make_unique<Dictionary>();
	dict->language = language;
	dict->scripts = LanguageScripts(language);
#ifdef _WIN32
	// The prefix makes hunspell assume the paths are UTF-8 and use _wfopen
	dict->hunspell = std::make_unique<Hunspell>(("\\\\?\\" + aff.string()).c_str(), ("\\\\?\\" + dic.string()).c_str());
#else
	dict->hunspell = std::make_unique<Hunspell>(aff.string().c_str(), dic.string().c_str());
#endif
	if (!dict->hunspell) return nullptr;

	dict->conv = std::make_unique<agi::charset::IconvWrapper>("utf-8", dict->hunspell->get_dic_encoding());
	dict->rconv = std::make_unique<agi::charset::IconvWrapper>(dict->hunspell->get_dic_encoding(), "utf-8");

	dict->userDicPath = config::path->Decode("?user/dictionaries")/agi::format("user_%s.dic", language);
	dict->ReadUserDictionary();

	for (auto const& word : dict->customWords) {
		try {
			dict->hunspell->add(dict->conv->Convert(word));
		}
		catch (agi::charset::ConvError const&) {
			// Normally this shouldn't happen, but some versions of Aegisub
			// wrote words in the wrong charset
		}
	}
	return dict;
}

void HunspellSpellChecker::OnLanguageChanged() {
	primary.reset();
	by_script.clear();

	auto language = OPT_GET("Tool/Spell Checker/Language")->GetString();
	enabled = !language.empty();
	if (enabled)
		primary = LoadDictionary(language);
}

void HunspellSpellChecker::OnPathChanged() {
	languages.clear();
	by_script.clear();
}

#endif // WITH_HUNSPELL
