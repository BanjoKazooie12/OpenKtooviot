// Copyright (c) 2026, Aegisub contributors
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
// Aegisub Project https://aegisub.org/

/// @file subs_edit_native.cpp
/// @brief Subtitle text editor using the native text box, for right-to-left text
/// @ingroup main_ui

#include "subs_edit_native.h"

#include "ass_dialogue.h"
#include "command/command.h"
#include "compat.h"
#include "format.h"
#include "include/aegisub/context.h"
#include "include/aegisub/spellchecker.h"
#include "options.h"
#include "selection_controller.h"
#include "text_selection_controller.h"
#include "utils.h"

#include <libaegisub/ass/dialogue_parser.h>
#include <libaegisub/character_count.h>
#include <libaegisub/spellchecker.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <unicode/uchar.h>
#include <unicode/utf8.h>

#include <wx/menu.h>

#include <optional>

#ifdef __WXGTK3__
#include <gtk/gtk.h>
#endif

namespace {
enum {
	ID_CUT = wxID_HIGHEST + 1,
	ID_COPY,
	ID_PASTE,
	ID_SELECT_ALL,
	ID_SPLIT_PRESERVE,
	ID_SPLIT_ESTIMATE,
	ID_SPLIT_VIDEO,
	ID_ADD_WORD,
	ID_REMOVE_WORD,
	ID_SUGGESTION,
	ID_SUGGESTION_LAST = ID_SUGGESTION + 31,
};

const wxChar left_to_right_mark = 0x200E;
const wxChar right_to_left_mark = 0x200F;

/// The direction of a line, from its first letter outside of override blocks
/// and escapes. Tags such as {\i1} start with Latin letters, so the text box's
/// own detection would make most tagged right-to-left lines left-to-right.
std::optional<bool> IsRightToLeft(std::string_view str) {
	int32_t i = 0, length = static_cast<int32_t>(str.size());
	while (i < length) {
		if (str[i] == '{') {
			auto close = str.find('}', i);
			if (close == std::string_view::npos) break;
			i = static_cast<int32_t>(close) + 1;
			continue;
		}
		if (str[i] == '\\' && i + 1 < length) {
			i += 2; // \N, \n, \h
			continue;
		}
		UChar32 c;
		U8_NEXT(str.data(), i, length, c);
		if (c < 0) continue;
		switch (u_charDirection(c)) {
			case U_LEFT_TO_RIGHT: return false;
			case U_RIGHT_TO_LEFT: case U_RIGHT_TO_LEFT_ARABIC: return true;
			default: break;
		}
	}
	return std::nullopt;
}

/// Real line breaks aren't valid in subtitle text; they become hard breaks
void ReplaceNewlines(std::string& str) {
	boost::replace_all(str, "\r\n", "\\N");
	boost::replace_all(str, "\n", "\\N");
	boost::replace_all(str, "\r", "\\N");
}

const char *SyntaxColourName(int type) {
	namespace ss = agi::ass::SyntaxStyle;
	switch (type) {
		case ss::COMMENT: return "Comment";
		case ss::DRAWING_CMD: return "Drawing Command";
		case ss::DRAWING_X: case ss::DRAWING_ENDPOINT_X: return "Drawing X";
		case ss::DRAWING_Y: case ss::DRAWING_ENDPOINT_Y: return "Drawing Y";
		case ss::OVERRIDE: return "Brackets";
		case ss::PUNCTUATION: return "Slashes";
		case ss::TAG: return "Tags";
		case ss::ERROR: return "Error";
		case ss::PARAMETER: return "Parameters";
		case ss::LINE_BREAK: return "Line Break";
		case ss::KARAOKE_TEMPLATE: return "Karaoke Template";
		case ss::KARAOKE_VARIABLE: return "Karaoke Variable";
		default: return nullptr;
	}
}

#ifdef __WXGTK3__
/// The GtkTextView inside a multiline wxTextCtrl
GtkTextView *TextView(wxTextCtrl *ctrl) {
	auto widget = static_cast<GtkWidget *>(ctrl->GetHandle());
	if (!widget) return nullptr;
	if (GTK_IS_TEXT_VIEW(widget)) return GTK_TEXT_VIEW(widget);
	if (GTK_IS_BIN(widget)) {
		GtkWidget *child = gtk_bin_get_child(GTK_BIN(widget));
		if (child && GTK_IS_TEXT_VIEW(child)) return GTK_TEXT_VIEW(child);
	}
	return nullptr;
}
#endif
}

SubsTextEditNative::SubsTextEditNative(wxWindow *parent, wxSize size, long style, agi::Context *context, bool right_to_left)
: wxTextCtrl(parent, -1, "", wxDefaultPosition, size, style | wxTE_MULTILINE | wxTE_RICH2 | wxTE_NOHIDESEL | wxTE_WORDWRAP)
, context(context)
, spellchecker(SpellCheckerFactory::GetSpellChecker())
, right_to_left(right_to_left)
, style_timer(this)
{
	SetLayoutDirection(right_to_left ? wxLayout_RightToLeft : wxLayout_LeftToRight);
#ifdef __WXGTK3__
	// wx sets the direction of the scrolled window around the text view, but
	// it's the text view's direction that decides where empty lines start
	if (auto view = TextView(this))
		gtk_widget_set_direction(GTK_WIDGET(view), right_to_left ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR);
#endif

	SetStyles();

	Bind(wxEVT_TEXT, &SubsTextEditNative::OnText, this);
	Bind(wxEVT_KEY_DOWN, &SubsTextEditNative::OnKeyDown, this);
	Bind(wxEVT_CONTEXT_MENU, &SubsTextEditNative::OnContextMenu, this);
	// There's no event for the cursor moving, so check when things settle
	Bind(wxEVT_IDLE, [this](wxIdleEvent& evt) { evt.Skip(); CheckSelection(); });
	Bind(wxEVT_TIMER, [this](wxTimerEvent&) { UpdateStyle(); }, style_timer.GetId());

	Bind(wxEVT_MENU, [this](wxCommandEvent&) { Cut(); }, ID_CUT);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { Copy(); }, ID_COPY);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { Paste(); }, ID_PASTE);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { SelectAll(); }, ID_SELECT_ALL);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) {
		spellchecker->AddWord(menu_word);
		UpdateStyle();
	}, ID_ADD_WORD);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) {
		spellchecker->RemoveWord(menu_word);
		UpdateStyle();
	}, ID_REMOVE_WORD);
	Bind(wxEVT_MENU, [this](wxCommandEvent& evt) {
		size_t index = evt.GetId() - ID_SUGGESTION;
		if (index < menu_suggestions.size())
			ReplaceRange(menu_word_start, menu_word_start + menu_word.size(), menu_suggestions[index]);
	}, ID_SUGGESTION, ID_SUGGESTION_LAST);
	if (context) {
		Bind(wxEVT_MENU, [=](wxCommandEvent&) { cmd::call("edit/line/split/preserve", context); }, ID_SPLIT_PRESERVE);
		Bind(wxEVT_MENU, [=](wxCommandEvent&) { cmd::call("edit/line/split/estimate", context); }, ID_SPLIT_ESTIMATE);
		Bind(wxEVT_MENU, [=](wxCommandEvent&) { cmd::call("edit/line/split/video", context); }, ID_SPLIT_VIDEO);
	}

	BindConnection(OPT_SUB("Subtitle/Edit Box/Font Face", &SubsTextEditNative::SetStyles, this));
	BindConnection(OPT_SUB("Subtitle/Edit Box/Font Size", &SubsTextEditNative::SetStyles, this));
	BindConnection(OPT_SUB("Colour/Subtitle/Background", &SubsTextEditNative::SetStyles, this));
	BindConnection(OPT_SUB("Colour/Subtitle/Syntax/Normal", &SubsTextEditNative::SetStyles, this));
	BindConnection(OPT_SUB("Subtitle/Highlight/Syntax", &SubsTextEditNative::UpdateStyle, this));
	BindConnection(OPT_SUB("Tool/Spell Checker/Language", [this] { style_timer.StartOnce(50); }));
}

SubsTextEditNative::~SubsTextEditNative() = default;

wxString SubsTextEditNative::Display(std::string const& str) const {
	bool rtl = IsRightToLeft(str).value_or(right_to_left);
	return wxString(rtl ? right_to_left_mark : left_to_right_mark) + to_wx(str);
}

long SubsTextEditNative::ToChar(int byte_pos) const {
	size_t pos = std::clamp<int>(byte_pos, 0, text.size());
	// Don't split a character
	while (pos > 0 && pos < text.size() && (static_cast<unsigned char>(text[pos]) & 0xC0) == 0x80)
		--pos;
	return 1 + to_wx(text.substr(0, pos)).length();
}

int SubsTextEditNative::ToByte(long char_pos) const {
	return from_wx(to_wx(text).Left(std::max(0L, char_pos - 1))).size();
}

std::pair<int, int> SubsTextEditNative::WordAt(int pos) const {
	auto tokens = agi::ass::TokenizeDialogueBody(text);
	agi::ass::SplitWords(text, tokens);
	int start = 0;
	for (auto const& token : tokens) {
		int end = start + static_cast<int>(token.length);
		if (token.type == agi::ass::DialogueTokenType::WORD && pos >= start && pos <= end)
			return {start, end};
		start = end;
	}
	return {pos, pos};
}

void SubsTextEditNative::SetStyles() {
	wxFont font = *wxNORMAL_FONT;
	font.SetEncoding(wxFONTENCODING_DEFAULT);
	wxString fontname = FontFace("Subtitle/Edit Box");
	if (!fontname.empty()) font.SetFaceName(fontname);
	font.SetPointSize(OPT_GET("Subtitle/Edit Box/Font Size")->GetInt());
	SetFont(font);

	auto background = to_wx(OPT_GET("Colour/Subtitle/Background")->GetColor());
	auto foreground = to_wx(OPT_GET("Colour/Subtitle/Syntax/Normal")->GetColor());
	SetBackgroundColour(background);
	SetForegroundColour(foreground);
	SetDefaultStyle(wxTextAttr(foreground, background, font));
	UpdateStyle();
}

void SubsTextEditNative::UpdateStyle() {
	style_timer.Stop();
	long length = GetLastPosition();
	if (length <= 0) return;

	wxTextAttr normal(to_wx(OPT_GET("Colour/Subtitle/Syntax/Normal")->GetColor()));
	normal.SetFontUnderlined(wxTEXT_ATTR_UNDERLINE_NONE);
	SetStyle(0, length, normal);

	if (!OPT_GET("Subtitle/Highlight/Syntax")->GetBool()) return;

	AssDialogue *line = context ? context->selectionController->GetActiveLine() : nullptr;
	bool template_line = line && line->Comment && boost::istarts_with(line->Effect.get(), "template");
	auto tokens = agi::ass::TokenizeDialogueBody(text, template_line);
	agi::ass::SplitWords(text, tokens);

	size_t pos = 0;
	for (auto const& range : agi::ass::SyntaxHighlight(text, tokens, spellchecker.get())) {
		long from = ToChar(pos), to = ToChar(pos + range.length);
		pos += range.length;
		if (from >= to) continue;

		if (range.type == agi::ass::SyntaxStyle::SPELLING) {
			wxTextAttr attr;
			attr.SetFontUnderlined(wxTEXT_ATTR_UNDERLINE_SPECIAL, *wxRED);
			SetStyle(from, to, attr);
		}
		else if (auto name = SyntaxColourName(range.type)) {
			wxTextAttr attr(to_wx(OPT_GET(std::string("Colour/Subtitle/Syntax/") + name)->GetColor()));
			SetStyle(from, to, attr);
		}
	}
}

void SubsTextEditNative::CheckSelection() {
	long from, to;
	GetSelection(&from, &to);
	long cursor = GetInsertionPoint();

	// Keep the cursor after the direction mark, so it can't be deleted or
	// typed in front of
	if (GetLastPosition() > 0 && (from == 0 || to == 0 || cursor == 0)) {
		if (from == to)
			SetInsertionPoint(1);
		else
			SetSelection(std::max(1L, from), std::max(1L, to));
		GetSelection(&from, &to);
		cursor = GetInsertionPoint();
	}

	if (from == last_from && to == last_to && cursor == last_cursor) return;
	last_from = from;
	last_to = to;
	last_cursor = cursor;
	if (selection_handler)
		selection_handler();
}

void SubsTextEditNative::SetTextTo(std::string const& new_text) {
	// Keep the cursor on the same character
	auto old_pos = agi::CharacterCount(std::string_view(text).substr(0, GetCursorPos()), 0);

	text = new_text;
	ChangeValue(Display(text));
	last_change = {};

	auto pos = static_cast<int>(agi::IndexOfCharacter(text, old_pos));
	if (context) {
		context->textSelectionController->SetSelection(0, 0);
		context->textSelectionController->SetSelection(pos, pos);
	}
	else
		SetSel(pos, pos);

	UpdateStyle();
}

int SubsTextEditNative::GetCursorPos() {
	return ToByte(GetInsertionPoint());
}

int SubsTextEditNative::GetSelStart() {
	long from, to;
	GetSelection(&from, &to);
	return ToByte(from);
}

int SubsTextEditNative::GetSelEnd() {
	long from, to;
	GetSelection(&from, &to);
	return ToByte(to);
}

void SubsTextEditNative::SetSel(int start, int end) {
	SetSelection(ToChar(start), ToChar(end));
	// The caller already knows where the selection is
	GetSelection(&last_from, &last_to);
	last_cursor = GetInsertionPoint();
}

void SubsTextEditNative::ReplaceRange(int start, int end, std::string const& replacement) {
	long from = ToChar(start);
	Replace(from, ToChar(end), to_wx(replacement));
	SetInsertionPoint(from + to_wx(replacement).length());
}

void SubsTextEditNative::OnText(wxCommandEvent&) {
	wxString value = GetValue();
	long cursor = GetInsertionPoint();

	// Take off the direction mark. If it's gone, the user deleted it.
	wxString body = value;
	if (!body.empty() && (body[0] == left_to_right_mark || body[0] == right_to_left_mark)) {
		body.erase(0, 1);
		--cursor;
	}
	std::string new_text = from_wx(body);
	// Pasted line breaks become hard breaks
	std::string before = from_wx(body.Left(std::max(0L, cursor)));
	ReplaceNewlines(before);
	ReplaceNewlines(new_text);

	// Restore the mark, which may also need to change direction, keeping the
	// cursor after the same text
	wxString display = Display(new_text);
	if (display != value) {
		ChangeValue(display);
		SetInsertionPoint(1 + to_wx(before).length());
		// Replacing the value loses the styling
		style_timer.StartOnce(1);
	}

	if (new_text == text) return;
	text = std::move(new_text);

	// Typing in quick succession is one undo step
	auto now = std::chrono::steady_clock::now();
	bool new_action = now - last_change > std::chrono::seconds(2);
	last_change = now;

	style_timer.StartOnce(150);
	CheckSelection();
	if (change_handler)
		change_handler(new_action);
}

void SubsTextEditNative::OnKeyDown(wxKeyEvent& evt) {
	int key = evt.GetKeyCode();
	bool enter = key == WXK_RETURN || key == WXK_NUMPAD_ENTER;
	bool linebreak = enter && evt.GetModifiers() == wxMOD_SHIFT;
#ifndef __WXMAC__
	bool hardspace = key == WXK_SPACE && evt.GetModifiers() == (wxMOD_CMD | wxMOD_SHIFT);
#else
	bool hardspace = key == WXK_SPACE && evt.GetModifiers() == wxMOD_ALT;
#endif

	if (linebreak || hardspace) {
		const char *insert = hardspace ? "\\h"
			: OPT_GET("Subtitle/Edit Box/Soft Line Break")->GetBool() ? "\\n" : "\\N";
		ReplaceRange(GetSelStart(), GetSelEnd(), insert);
		return;
	}

	// Enter is handled by the hotkeys; it must never insert a real line break
	if (enter) return;

	evt.Skip();
}

void SubsTextEditNative::OnContextMenu(wxContextMenuEvent& evt) {
	int pos = GetCursorPos();
#ifdef __WXGTK3__
	// Spelling suggestions are for the word that was clicked, not the one
	// the cursor is in. The event's position is mirrored in right-to-left
	// windows, so ask GTK where the pointer is instead.
	auto view = TextView(this);
	if (view && evt.GetPosition() != wxDefaultPosition) {
		GdkWindow *window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
		GdkSeat *seat = gdk_display_get_default_seat(gtk_widget_get_display(GTK_WIDGET(view)));
		GdkDevice *pointer = seat ? gdk_seat_get_pointer(seat) : nullptr;
		if (window && pointer) {
			int x, y, buffer_x, buffer_y;
			gdk_window_get_device_position(window, pointer, &x, &y, nullptr);
			gtk_text_view_window_to_buffer_coords(view, GTK_TEXT_WINDOW_TEXT, x, y, &buffer_x, &buffer_y);
			GtkTextIter iter;
			if (gtk_text_view_get_iter_at_location(view, &iter, buffer_x, buffer_y))
				pos = ToByte(gtk_text_iter_get_offset(&iter));
		}
	}
#endif

	auto [word_start, word_end] = WordAt(pos);
	menu_word = text.substr(word_start, word_end - word_start);
	menu_word_start = word_start;
	menu_suggestions.clear();

	wxMenu menu;
	if (spellchecker && !menu_word.empty()) {
		if (!spellchecker->CheckWord(menu_word)) {
			menu_suggestions = spellchecker->GetSuggestions(menu_word);
			if (menu_suggestions.size() > ID_SUGGESTION_LAST - ID_SUGGESTION + 1)
				menu_suggestions.resize(ID_SUGGESTION_LAST - ID_SUGGESTION + 1);
			if (menu_suggestions.empty())
				menu.Append(-1, _("No correction suggestions"))->Enable(false);
			for (size_t i = 0; i < menu_suggestions.size(); ++i)
				menu.Append(ID_SUGGESTION + i, to_wx(menu_suggestions[i]));
			menu.Append(ID_ADD_WORD, fmt_tl("Add \"%s\" to dictionary", menu_word))->Enable(spellchecker->CanAddWord(menu_word));
			menu.AppendSeparator();
		}
		else if (spellchecker->CanRemoveWord(menu_word)) {
			menu.Append(ID_REMOVE_WORD, fmt_tl("Remove \"%s\" from dictionary", menu_word));
			menu.AppendSeparator();
		}
	}

	long from, to;
	GetSelection(&from, &to);
	menu.Append(ID_CUT, _("Cu&t"))->Enable(from != to);
	menu.Append(ID_COPY, _("&Copy"))->Enable(from != to);
	menu.Append(ID_PASTE, _("&Paste"))->Enable(CanPaste());
	menu.AppendSeparator();
	menu.Append(ID_SELECT_ALL, _("Select &All"));

	if (context) {
		menu.AppendSeparator();
		menu.Append(ID_SPLIT_PRESERVE, _("Split at cursor (preserve times)"));
		menu.Append(ID_SPLIT_ESTIMATE, _("Split at cursor (estimate times)"));
		cmd::Command *split_video = cmd::get("edit/line/split/video");
		menu.Append(ID_SPLIT_VIDEO, split_video->StrMenu(context))->Enable(split_video->Validate(context));
	}

	PopupMenu(&menu);
}
