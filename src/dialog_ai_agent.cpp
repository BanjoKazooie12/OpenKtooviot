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

/// @file dialog_ai_agent.cpp
/// @brief Dialog for translating subtitles with the AI agent
/// @ingroup secondary_ui

#include "ai_agent.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "compat.h"
#include "dialog_manager.h"
#include "dialogs.h"
#include "format.h"
#include "include/aegisub/context.h"
#include "libresrc/libresrc.h"
#include "options.h"
#include "project.h"
#include "selection_controller.h"

#include <libaegisub/dispatch.h>

#include <boost/algorithm/string/trim.hpp>

#include <map>
#include <memory>
#include <thread>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace {

class DialogAIAgent final : public wxDialog, public ai::AgentListener {
	agi::Context *c;

	wxChoice *provider;
	wxComboBox *model;
	/// The provider whose model is shown in the model box
	ai::Provider shown_provider = ai::Provider::Claude;
	/// Lets background tasks know whether the dialog still exists
	std::shared_ptr<bool> alive = std::make_shared<bool>(true);

	wxComboBox *language;
	wxRadioBox *scope;
	wxCheckBox *use_video;
	wxTextCtrl *instructions;
	wxButton *translate_button;
	wxButton *stop_button;
	wxTextCtrl *log;
	wxTextCtrl *chat;
	wxButton *send_button;
	wxButton *new_button;
	wxStaticText *usage_label;

	/// Destroyed before the controls, which cancels any running request
	std::unique_ptr<ai::Agent> agent;

	void Append(wxString const& text, wxColour const& colour, bool bold = false);
	void SetBusy(bool busy);
	bool Start(std::string const& message);
	void SaveSettings();
	ai::Provider SelectedProvider() const;
	void ShowModels();
	void OnProviderChanged();

	void OnTranslate();
	void OnSend();
	void OnNewConversation();

	// AgentListener
	void OnAgentText(std::string const& text) override;
	void OnAgentActivity(std::string const& text) override;
	void OnAgentUsage(ai::AgentUsage const& usage) override;
	void OnAgentFinished(std::string const& error) override;

public:
	DialogAIAgent(agi::Context *c);
};

DialogAIAgent::DialogAIAgent(agi::Context *c)
: wxDialog(c->parent, -1, _("AI Translation Agent"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
, c(c)
{
	SetIcons(GETICONS(automation_toolbutton));

	// Translation settings
	wxString languages[] = {
		_("Hebrew"), _("English"), _("Arabic"), _("Spanish"), _("French"), _("German"),
		_("Italian"), _("Portuguese"), _("Russian"), _("Ukrainian"), _("Polish"), _("Turkish"),
		_("Persian"), _("Hindi"), _("Japanese"), _("Korean"), _("Chinese (Simplified)"),
		_("Chinese (Traditional)"), _("Indonesian"), _("Vietnamese"), _("Thai"), _("Dutch"), _("Greek"),
	};
	auto settings_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Translation"));
	auto box = settings_box->GetStaticBox();

	wxString providers[] = { _("Claude (Anthropic API)"), _("Ollama (local models)") };
	provider = new wxChoice(box, -1, wxDefaultPosition, wxDefaultSize, std::size(providers), providers);
	provider->SetSelection(OPT_GET("Automation/AI Agent/Provider")->GetString() == "ollama" ? 1 : 0);
	shown_provider = SelectedProvider();
	model = new wxComboBox(box, -1, "", wxDefaultPosition, wxSize(220, -1));
	model->SetToolTip(_("The model to use. For Ollama, models you have installed are listed; install more with \"ollama pull\"."));

	auto service_row = new wxFlexGridSizer(2, 5, 5);
	service_row->AddGrowableCol(1, 1);
	service_row->Add(new wxStaticText(box, -1, _("AI service:")), wxSizerFlags().Center());
	service_row->Add(provider, wxSizerFlags(1).Expand());
	service_row->Add(new wxStaticText(box, -1, _("Model:")), wxSizerFlags().Center());
	service_row->Add(model, wxSizerFlags(1).Expand());
	settings_box->Add(service_row, wxSizerFlags().Expand().Border(wxALL & ~wxBOTTOM));

	wxString target = to_wx(OPT_GET("Automation/AI Agent/Target Language")->GetString());
	language = new wxComboBox(box, -1, target.empty() ? languages[0] : target,
		wxDefaultPosition, wxSize(220, -1), std::size(languages), languages);
	language->SetToolTip(_("Choose a language or type any other"));

	wxString scopes[] = { _("Selected lines"), _("All lines") };
	scope = new wxRadioBox(box, -1, _("Lines to translate"), wxDefaultPosition, wxDefaultSize, std::size(scopes), scopes, 1);
	scope->SetSelection(std::clamp<int>(OPT_GET("Automation/AI Agent/Scope")->GetInt(), 0, 1));

	use_video = new wxCheckBox(box, -1, _("Let the agent look at the &video"));
	use_video->SetValue(OPT_GET("Automation/AI Agent/Use Video Frames")->GetBool());
	use_video->SetToolTip(_("The agent can look at frames of the loaded video, for example to see who is speaking. Each frame adds to the cost of the request."));

	instructions = new wxTextCtrl(box, -1, to_wx(OPT_GET("Automation/AI Agent/Instructions")->GetString()),
		wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE);
	instructions->SetHint(_("Optional: tone, names, terms to keep untranslated..."));

	auto language_row = new wxBoxSizer(wxHORIZONTAL);
	language_row->Add(new wxStaticText(box, -1, _("Translate into:")), wxSizerFlags().Center().Border(wxRIGHT));
	language_row->Add(language, wxSizerFlags(1).Center());

	settings_box->Add(language_row, wxSizerFlags().Expand().Border(wxALL & ~wxBOTTOM));
	settings_box->Add(scope, wxSizerFlags().Expand().Border());
	settings_box->Add(use_video, wxSizerFlags().Border(wxLEFT | wxRIGHT));
	settings_box->Add(new wxStaticText(box, -1, _("Extra instructions:")), wxSizerFlags().Border(wxALL & ~wxBOTTOM));
	settings_box->Add(instructions, wxSizerFlags().Expand().Border());

	translate_button = new wxButton(this, -1, _("&Translate"));
	stop_button = new wxButton(this, -1, _("St&op"));
	auto action_row = new wxBoxSizer(wxHORIZONTAL);
	action_row->AddStretchSpacer();
	action_row->Add(stop_button, wxSizerFlags().Border(wxRIGHT));
	action_row->Add(translate_button);

	// Conversation
	auto conversation_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Conversation"));
	box = conversation_box->GetStaticBox();
	log = new wxTextCtrl(box, -1, "", wxDefaultPosition, wxSize(520, 260), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_WORDWRAP);
	chat = new wxTextCtrl(box, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	chat->SetHint(_("Ask for changes, e.g. \"make row 12 more formal\""));
	send_button = new wxButton(box, -1, _("&Send"));
	auto chat_row = new wxBoxSizer(wxHORIZONTAL);
	chat_row->Add(chat, wxSizerFlags(1).Center().Border(wxRIGHT));
	chat_row->Add(send_button, wxSizerFlags().Center());
	conversation_box->Add(log, wxSizerFlags(1).Expand().Border());
	conversation_box->Add(chat_row, wxSizerFlags().Expand().Border(wxALL & ~wxTOP));

	usage_label = new wxStaticText(this, -1, "");
	new_button = new wxButton(this, -1, _("&New conversation"));
	auto bottom_row = new wxBoxSizer(wxHORIZONTAL);
	bottom_row->Add(usage_label, wxSizerFlags(1).Center());
	bottom_row->Add(new_button, wxSizerFlags().Border(wxRIGHT));
	bottom_row->Add(new wxButton(this, wxID_CANCEL, _("&Close")));

	auto main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(settings_box, wxSizerFlags().Expand().Border());
	main_sizer->Add(action_row, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT));
	main_sizer->Add(conversation_box, wxSizerFlags(1).Expand().Border());
	main_sizer->Add(bottom_row, wxSizerFlags().Expand().Border(wxALL & ~wxTOP));
	SetSizerAndFit(main_sizer);
	CenterOnParent();

	agent = std::make_unique<ai::Agent>(c, this);
	ShowModels();

	provider->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { OnProviderChanged(); });
	translate_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnTranslate(); });
	stop_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		agent->Cancel();
		stop_button->Enable(false);
	});
	send_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnSend(); });
	chat->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnSend(); });
	new_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnNewConversation(); });

	SetBusy(false);
	translate_button->SetDefault();
}

void DialogAIAgent::Append(wxString const& text, wxColour const& colour, bool bold) {
	wxTextAttr attr(colour.IsOk() ? colour : log->GetForegroundColour());
	wxFont font = log->GetFont();
	if (bold) font.MakeBold();
	attr.SetFont(font);
	log->SetDefaultStyle(attr);
	if (!log->IsEmpty())
		log->AppendText("\n");
	log->AppendText(text);
	log->ShowPosition(log->GetLastPosition());
}

void DialogAIAgent::SetBusy(bool busy) {
	provider->Enable(!busy);
	model->Enable(!busy);
	translate_button->Enable(!busy);
	send_button->Enable(!busy);
	new_button->Enable(!busy);
	stop_button->Enable(busy);
}

ai::Provider DialogAIAgent::SelectedProvider() const {
	return provider->GetSelection() == 1 ? ai::Provider::Ollama : ai::Provider::Claude;
}

void DialogAIAgent::ShowModels() {
	if (shown_provider == ai::Provider::Claude) {
		wxString claude_models[] = { "claude-opus-5-5", "claude-sonnet-5-5", "claude-haiku-5-5" };
		model->Set(std::size(claude_models), claude_models);
		model->SetValue(to_wx(OPT_GET("Automation/AI Agent/Model")->GetString()));
		return;
	}

	model->Clear();
	model->SetValue(to_wx(OPT_GET("Automation/AI Agent/Ollama Model")->GetString()));

	// Listing the models is a network request, so do it in the background
	std::weak_ptr<bool> weak = alive;
	std::string url = OPT_GET("Automation/AI Agent/Ollama URL")->GetString();
	std::thread([this, weak, url] {
		std::string error;
		auto models = ai::ListOllamaModels(url, error);
		if (weak.expired()) return;
		agi::dispatch::Main().Async([this, weak, models, error, url] {
			if (weak.expired() || shown_provider != ai::Provider::Ollama) return;
			if (!error.empty()) {
				Append(fmt_tl("Could not get the list of models from Ollama at %s. Is Ollama running? (%s)", url, error), wxColour(128, 128, 128));
				return;
			}
			wxString current = model->GetValue();
			wxArrayString names;
			for (auto const& name : models)
				names.push_back(to_wx(name));
			model->Set(names);
			if (current.empty() && !names.empty())
				current = names[0];
			model->SetValue(current);
			if (names.empty())
				Append(_("No models are installed in Ollama yet. Install one with \"ollama pull\", for example: ollama pull qwen3-vl"), wxColour(128, 128, 128));
		});
	}).detach();
}

void DialogAIAgent::OnProviderChanged() {
	if (agent->IsRunning()) {
		provider->SetSelection(shown_provider == ai::Provider::Ollama ? 1 : 0);
		return;
	}
	SaveSettings();
	shown_provider = SelectedProvider();
	OPT_SET("Automation/AI Agent/Provider")->SetString(shown_provider == ai::Provider::Ollama ? "ollama" : "claude");
	if (!log->IsEmpty()) {
		OnNewConversation();
		Append(_("Started a new conversation for the new AI service."), wxColour(128, 128, 128));
	}
	ShowModels();
}

void DialogAIAgent::SaveSettings() {
	auto model_name = from_wx(model->GetValue().Strip(wxString::both));
	if (shown_provider == ai::Provider::Ollama)
		OPT_SET("Automation/AI Agent/Ollama Model")->SetString(model_name);
	else if (!model_name.empty())
		OPT_SET("Automation/AI Agent/Model")->SetString(model_name);
	OPT_SET("Automation/AI Agent/Target Language")->SetString(from_wx(language->GetValue()));
	OPT_SET("Automation/AI Agent/Scope")->SetInt(scope->GetSelection());
	OPT_SET("Automation/AI Agent/Use Video Frames")->SetBool(use_video->GetValue());
	OPT_SET("Automation/AI Agent/Instructions")->SetString(from_wx(instructions->GetValue()));
}

bool DialogAIAgent::Start(std::string const& message) {
	ai::AgentSettings settings;
	settings.provider = SelectedProvider();
	settings.model = from_wx(model->GetValue());
	boost::trim(settings.model);

	if (settings.provider == ai::Provider::Claude) {
		settings.api_key = ai::GetApiKey();
		if (settings.api_key.empty()) {
			int result = wxMessageBox(
				_("The AI agent needs a Claude API key. Add one in Preferences > Automation, or set the ANTHROPIC_API_KEY environment variable.\n\nOpen Preferences now?"),
				_("No API key"), wxYES_NO | wxICON_INFORMATION, this);
			if (result == wxYES)
				ShowPreferences(this);
			return false;
		}
		if (settings.model.empty())
			settings.model = "claude-opus-5-5";
		settings.effort = OPT_GET("Automation/AI Agent/Effort")->GetString();
	}
	else {
		if (settings.model.empty()) {
			wxMessageBox(_("Choose an Ollama model first. If the list is empty, install a model with \"ollama pull\", for example: ollama pull qwen3-vl"),
				_("AI Translation Agent"), wxOK | wxICON_INFORMATION, this);
			return false;
		}
		settings.server_url = OPT_GET("Automation/AI Agent/Ollama URL")->GetString();
		boost::trim(settings.server_url);
		settings.context_length = OPT_GET("Automation/AI Agent/Ollama Context Length")->GetInt();
	}

	settings.max_frames = use_video->GetValue() && c->project->VideoProvider() && !ai::IsPlaceholder(c->project->VideoName())
		? std::max<int>(0, OPT_GET("Automation/AI Agent/Max Frames")->GetInt())
		: 0;

	SetBusy(true);
	agent->Send(message, settings);
	return true;
}

void DialogAIAgent::OnTranslate() {
	SaveSettings();

	wxString target = language->GetValue().Strip(wxString::both);
	if (target.empty()) {
		wxMessageBox(_("Choose the language to translate into."), _("AI Translation Agent"), wxOK | wxICON_INFORMATION, this);
		return;
	}

	std::map<AssDialogue *, int> rows;
	int row = 0;
	for (auto& line : c->ass->Events)
		rows[&line] = ++row;

	std::string lines;
	if (scope->GetSelection() == 0) {
		std::vector<int> selected;
		for (auto line : c->selectionController->GetSelectedSet()) {
			auto it = rows.find(line);
			if (it != rows.end()) selected.push_back(it->second);
		}
		if (selected.empty()) {
			wxMessageBox(_("Select the lines to translate in the subtitle grid first, or choose \"All lines\"."), _("AI Translation Agent"), wxOK | wxICON_INFORMATION, this);
			return;
		}
		std::sort(selected.begin(), selected.end());
		std::string list;
		for (size_t i = 0; i < selected.size(); ) {
			size_t j = i;
			while (j + 1 < selected.size() && selected[j + 1] == selected[j] + 1) ++j;
			if (!list.empty()) list += ", ";
			list += std::to_string(selected[i]);
			if (j > i) list += "-" + std::to_string(selected[j]);
			i = j + 1;
		}
		lines = "the lines the user selected: rows " + list;
	}
	else
		lines = "every dialogue line in the file (rows 1-" + std::to_string(row) + ")";

	std::string message = "Translate " + lines + " into " + from_wx(target) + ".";
	auto extra = from_wx(instructions->GetValue());
	boost::trim(extra);
	if (!extra.empty())
		message += "\n\nInstructions from the user:\n" + extra;
	if (!use_video->GetValue())
		message += "\n\nThe user asked you not to look at the video for this request.";
	else if (!c->project->VideoProvider() || ai::IsPlaceholder(c->project->VideoName()))
		message += "\n\nNo real video is loaded, so translate from the text alone.";

	if (Start(message)) {
		Append(scope->GetSelection() == 0
			? fmt_tl("Translate the selected lines into %s", target)
			: fmt_tl("Translate all lines into %s", target), wxNullColour, true);
	}
}

void DialogAIAgent::OnSend() {
	if (agent->IsRunning()) return;
	auto text = chat->GetValue().Strip(wxString::both);
	if (text.empty()) return;
	SaveSettings();

	std::string message = from_wx(text);
	if (!use_video->GetValue())
		message += "\n\n(The user asked you not to look at the video for this request.)";
	if (Start(message)) {
		Append(text, wxNullColour, true);
		chat->Clear();
	}
}

void DialogAIAgent::OnNewConversation() {
	agent->Reset();
	log->Clear();
	usage_label->SetLabel("");
}

void DialogAIAgent::OnAgentText(std::string const& text) {
	Append(to_wx(text), wxNullColour);
}

void DialogAIAgent::OnAgentActivity(std::string const& text) {
	Append(L"• " + to_wx(text), wxColour(128, 128, 128));
}

void DialogAIAgent::OnAgentUsage(ai::AgentUsage const& usage) {
	int64_t input = usage.input_tokens + usage.cache_read_tokens + usage.cache_write_tokens;
	if (usage.cache_read_tokens > 0)
		usage_label->SetLabel(fmt_tl("Tokens: %d in (%d cached), %d out",
			(long long)input, (long long)usage.cache_read_tokens, (long long)usage.output_tokens));
	else
		usage_label->SetLabel(fmt_tl("Tokens: %d in, %d out", (long long)input, (long long)usage.output_tokens));
}

void DialogAIAgent::OnAgentFinished(std::string const& error) {
	if (error.empty())
		Append(_("Done."), wxColour(128, 128, 128));
	else
		Append(to_wx(error), *wxRED);
	SetBusy(false);
}

}

void ShowAIAgentDialog(agi::Context *c) {
	c->dialog->Show<DialogAIAgent>(c);
}
