// Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "text_selection_controller.h"

#include "subs_text_editor.h"

void TextSelectionController::SetControl(SubsTextEditor *ctrl) {
	if (this->ctrl)
		this->ctrl->SetSelectionHandler(nullptr);
	this->ctrl = ctrl;
	if (ctrl)
		ctrl->SetSelectionHandler([this] { UpdateUI(); });
}

TextSelectionController::~TextSelectionController() {
	if (ctrl) ctrl->SetSelectionHandler(nullptr);
}

#define GET(var, new_value) do { \
	int tmp = new_value;      \
	if (tmp != var) {         \
		var = tmp;            \
		changed = true;       \
	}                         \
} while(false)

void TextSelectionController::UpdateUI() {
	if (changing || !ctrl) return;

	bool changed = false;
	GET(insertion_point, ctrl->GetCursorPos());
	GET(selection_start, ctrl->GetSelStart());
	GET(selection_end, ctrl->GetSelEnd());
	if (changed) AnnounceSelectionChanged();
}

void TextSelectionController::SetInsertionPoint(int position) {
	changing = true;
	if (insertion_point != position) {
		insertion_point = position;
		if (ctrl) ctrl->SetSel(position, position);
	}
	changing = false;
	AnnounceSelectionChanged();
}

void TextSelectionController::SetSelection(int start, int end) {
	changing = true;
	if (selection_start != start || selection_end != end) {
		selection_start = start;
		selection_end = end;
		if (ctrl) ctrl->SetSel(start, end);
	}
	changing = false;
	AnnounceSelectionChanged();
}
