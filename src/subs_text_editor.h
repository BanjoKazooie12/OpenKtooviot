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

/// @file subs_text_editor.h
/// @brief Interface to the editor for the text of the active line
/// @ingroup main_ui

#pragma once

#include <functional>
#include <string>

class wxWindow;

/// The editor for the text of the active line. It's either the Scintilla
/// editor, which has syntax highlighting and call tips, or a native text box,
/// which supports right-to-left text.
///
/// Positions are byte offsets into the UTF-8 text.
class SubsTextEditor {
public:
	virtual ~SubsTextEditor() = default;

	virtual wxWindow *GetEditorWindow() = 0;

	/// Replace the text, keeping the cursor on the same character if possible
	virtual void SetTextTo(std::string const& text) = 0;
	virtual std::string GetEditorText() = 0;

	virtual int GetCursorPos() = 0;
	virtual int GetSelStart() = 0;
	virtual int GetSelEnd() = 0;
	virtual void SetSel(int start, int end) = 0;

	/// Called when the user changes the text. The argument is true if the
	/// change starts a new action for the purposes of undo.
	virtual void SetChangeHandler(std::function<void(bool)> handler) = 0;
	/// Called when the cursor or selection may have moved
	virtual void SetSelectionHandler(std::function<void()> handler) = 0;
};
