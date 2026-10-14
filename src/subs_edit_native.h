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

/// @file subs_edit_native.h
/// @see subs_edit_native.cpp
/// @ingroup main_ui

#pragma once

#include "subs_text_editor.h"

#include <libaegisub/signal.h>

#include <chrono>
#include <memory>
#include <vector>

#include <wx/textctrl.h>
#include <wx/timer.h>

namespace agi {
	class SpellChecker;
	struct Context;
	namespace ass { struct DialogueToken; }
}

/// A subtitle text editor built on the platform's native text box. Unlike the
/// Scintilla editor, it lays out right-to-left text properly, so it's used for
/// right-to-left languages. It underlines misspellings and colours override
/// tags, but has no call tips.
class SubsTextEditNative final : public wxTextCtrl, public SubsTextEditor, private agi::signal::ConnectionScope {
	agi::Context *context;
	std::unique_ptr<agi::SpellChecker> spellchecker;
	/// Direction of lines with no letters, such as empty ones
	bool right_to_left;

	std::function<void(bool)> change_handler;
	std::function<void()> selection_handler;

	/// The text as UTF-8. The text box shows it after an invisible direction
	/// mark (see Display()), so character positions in the text box are one
	/// more than in the text.
	std::string text;

	/// What the text box shows for some text
	wxString Display(std::string const& str) const;
	/// Last known selection, in characters, to notice when it moves
	long last_from = -1, last_to = -1, last_cursor = -1;
	/// When the user last changed the text, to group typing into undo steps
	std::chrono::steady_clock::time_point last_change;

	/// Restyles the text a moment after it stops changing
	wxTimer style_timer;

	/// Context menu state
	std::string menu_word;
	int menu_word_start = 0;
	std::vector<std::string> menu_suggestions;

	long ToChar(int byte_pos) const;
	int ToByte(long char_pos) const;

	/// Bounds in bytes of the word at a byte position, or {pos, pos}
	std::pair<int, int> WordAt(int pos) const;

	void SetStyles();
	void UpdateStyle();
	void CheckSelection();
	void ReplaceRange(int start, int end, std::string const& replacement);

	void OnText(wxCommandEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnContextMenu(wxContextMenuEvent& evt);

public:
	SubsTextEditNative(wxWindow *parent, wxSize size, long style, agi::Context *context, bool right_to_left);
	~SubsTextEditNative();

	// SubsTextEditor
	wxWindow *GetEditorWindow() override { return this; }
	void SetTextTo(std::string const& text) override;
	std::string GetEditorText() override { return text; }
	int GetCursorPos() override;
	int GetSelStart() override;
	int GetSelEnd() override;
	void SetSel(int start, int end) override;
	void SetChangeHandler(std::function<void(bool)> handler) override { change_handler = std::move(handler); }
	void SetSelectionHandler(std::function<void()> handler) override { selection_handler = std::move(handler); }
};
