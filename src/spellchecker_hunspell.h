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
// Aegisub Project http://www.aegisub.org/

/// @file spellchecker_hunspell.h
/// @see spellchecker_hunspell.cpp
/// @ingroup spelling
///

#pragma once

#ifdef WITH_HUNSPELL
#include <libaegisub/spellchecker.h>

#include <libaegisub/fs.h>
#include <libaegisub/signal.h>

#include <map>
#include <memory>
#include <set>

namespace agi { namespace charset { class IconvWrapper; } }
class Hunspell;

class HunspellSpellChecker final : public agi::SpellChecker {
	/// A loaded dictionary and the user's additions to it
	struct Dictionary {
		std::string language;
		std::unique_ptr<Hunspell> hunspell;

		/// Conversions between the dictionary charset and utf-8
		std::unique_ptr<agi::charset::IconvWrapper> conv;
		std::unique_ptr<agi::charset::IconvWrapper> rconv;

		/// Writing systems (ICU UScriptCodes) of the dictionary's language.
		/// Empty if they couldn't be determined.
		std::vector<int> scripts;

		/// Path to user-local dictionary.
		agi::fs::path userDicPath;

		/// Words in the custom user dictionary
		std::set<std::string, std::less<>> customWords;

		~Dictionary();
		bool Covers(int script) const;
		void ReadUserDictionary();
		void WriteUserDictionary() const;
	};

	/// The dictionary the user chose
	std::unique_ptr<Dictionary> primary;

	/// False if the user turned spell checking off
	bool enabled = false;

	/// Dictionaries for words written in a script the chosen dictionary can't
	/// check, by script. A null entry means there's no dictionary for it.
	std::map<int, std::unique_ptr<Dictionary>> by_script;

	/// Languages which we have dictionaries for
	std::vector<std::string> languages;

	/// Dictionary language change connection
	agi::signal::Connection lang_listener;
	/// Dictionary language change handler
	void OnLanguageChanged();

	/// Dictionary path change connection
	agi::signal::Connection dict_path_listener;
	/// Dictionary path change handler
	void OnPathChanged();

	std::unique_ptr<Dictionary> LoadDictionary(std::string const& language);

	/// The dictionary to check the word with, or null if no available
	/// dictionary can check it (it's in another writing system)
	Dictionary *DictionaryFor(std::string_view word);

	/// Save a dictionary's custom words, and tell other spell checkers
	void SaveUserWords(Dictionary const& dict);

public:
	HunspellSpellChecker();
	~HunspellSpellChecker();

	void AddWord(std::string_view word) override;
	void RemoveWord(std::string_view word) override;
	bool CanAddWord(std::string_view word) override;
	bool CanRemoveWord(std::string_view word) override;
	bool CheckWord(std::string_view word) override;
	std::vector<std::string> GetSuggestions(std::string_view word) override;
	std::vector<std::string> GetLanguageList() override;
};

#endif
