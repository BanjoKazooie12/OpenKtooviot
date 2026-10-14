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

/// @file ai_agent.cpp
/// @brief Agent which edits the open subtitles through the Claude API
/// @ingroup ai

#include "ai_agent.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "async_video_provider.h"
#include "compat.h"
#include "format.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "project.h"
#include "selection_controller.h"
#include "version.h"
#include "video_controller.h"
#include "video_frame.h"

#include <libaegisub/cajun/elements.h>
#include <libaegisub/cajun/reader.h>
#include <libaegisub/cajun/writer.h>
#include <libaegisub/dispatch.h>
#include <libaegisub/log.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <curl/curl.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <map>
#include <optional>
#include <sstream>
#include <thread>

#include <wx/base64.h>
#include <wx/image.h>
#include <wx/imagjpeg.h>
#include <wx/intl.h>
#include <wx/mstream.h>

namespace {
const char default_base_url[] = "https://api.anthropic.com";
const char fallback_beta[] = "server-side-fallback-2026-07-01";
/// Upper bound on model requests for a single user message
const int max_requests = 60;
const int max_output_tokens = 16000;
const int max_retries = 4;
const size_t max_read_rows = 200;
const size_t max_edit_rows = 200;
const int frame_max_dimension = 1024;

const char system_prompt[] = R"(You are a subtitle translation assistant built into Aegisub, a subtitle editor. You work directly on the subtitle file the user has open, using the tools provided. Edits you make appear in the editor immediately, and the user can undo each of your requests with a single Ctrl+Z.

Subtitle lines are identified by row numbers, which match the "#" column of Aegisub's subtitle grid (1 is the first line of the file).

How to work:
- Start with get_project_info unless you already know the state of the file, then read the lines you were asked to work on with read_lines. Read a few lines before and after them when context would help the translation.
- Write translations with set_line_texts, in batches of up to about 50 lines, and continue until every requested line is done. Do not stop to ask for confirmation unless the request is genuinely ambiguous.
- Only change the lines you were asked to change. Skip comment lines unless the user asks for them.
- Finish with a short summary in the user's language of the conversation: what you changed, and any rows where you were unsure (for example names, wordplay, or a speaker you could not identify).

The text of each line is in Advanced SubStation Alpha (ASS) format:
- Override blocks such as {\i1}, {\an8}, {\pos(320,50)} or {\k20} are formatting, not text. Keep every override block unchanged and place it at the position in the translation that corresponds to its position in the original (an {\i1}...{\i0} pair should surround the translated words). Never translate or modify anything inside braces.
- \N is a hard line break and \n a soft one; keep them, rebalancing where the break falls so both lines read naturally. \h is a hard space.
- Karaoke lines contain \k, \kf or \ko tags before each syllable. When translating them, keep the tags in order around the translated syllables or words.
- Lines that contain drawing commands (\p1 and above) are vector drawings; leave them unchanged.

Translation quality:
- Translate meaning and tone, not word for word. Keep each subtitle short enough to read in the time it is on screen: roughly 15 to 20 characters per second.
- Keep names consistent across the file. Keep honorifics and terms the way the user asks; otherwise choose what a professional subtitler would.
- When the target language needs information the text does not give, such as the gender of the speaker or the person addressed (Hebrew, Arabic, French, Spanish, Russian and many others), or the level of formality, look at the video with get_video_frame for that line. Use the video too for on-screen text and signs, and to tell who is speaking. Each frame you look at costs time and money, so look only when it can change the translation, and never more than once for the same moment.)";

const char tools_json[] = R"([
{"name":"get_project_info","description":"Get an overview of the open subtitle file: title, resolution, number of lines, styles, the loaded video and audio, which rows the user has selected, and how many video frames you may still look at during this request. Call this at the start of a task.","input_schema":{"type":"object","properties":{}}},
{"name":"read_lines","description":"Read subtitle lines by row number (inclusive range, at most 200 rows per call). Returns each line's row, start and end time, style, actor, whether it is a comment, and its raw ASS text including override tags. Call this before translating or editing lines.","input_schema":{"type":"object","properties":{"first_row":{"type":"integer","description":"First row to read, starting at 1"},"last_row":{"type":"integer","description":"Last row to read"}},"required":["first_row","last_row"]}},
{"name":"set_line_texts","description":"Replace the text of one or more subtitle lines (at most 200 per call). Use this to write translations or corrections. The text must be ASS text: keep override blocks in braces and \\N line breaks. Only the text changes; times, styles and other fields are kept.","input_schema":{"type":"object","properties":{"edits":{"type":"array","description":"The lines to change","items":{"type":"object","properties":{"row":{"type":"integer","description":"Row number of the line"},"text":{"type":"string","description":"New ASS text for the line"}},"required":["row","text"]}}},"required":["edits"]}},
{"name":"get_video_frame","description":"Look at a frame of the loaded video, as a JPEG image. Give a row to see the middle of that subtitle line, or a time in milliseconds. Use this when the image can change the translation: to find out who is speaking or being addressed (for gendered languages), the setting or formality, or on-screen text. Frames are limited per request; get_project_info tells you how many remain.","input_schema":{"type":"object","properties":{"row":{"type":"integer","description":"Row of the line to look at"},"time_ms":{"type":"integer","description":"Time to look at in milliseconds, if no row is given"},"with_subtitles":{"type":"boolean","description":"Draw the subtitles onto the frame. Defaults to false."}}}}
])";

/// Models which accept the "default" form of server-side fallbacks
bool SupportsDefaultFallback(std::string const& model) {
	for (auto m : {"claude-opus-5-5", "claude-opus-5", "claude-fable-5-1", "claude-sonnet-5-5"}) {
		if (model == m) return true;
	}
	return false;
}

template<typename T>
T const *Get(json::Object const& obj, const char *key) {
	auto it = obj.find(key);
	if (it == obj.end()) return nullptr;
	try {
		return &static_cast<T const&>(it->second);
	}
	catch (json::Exception const&) {
		return nullptr;
	}
}

std::optional<int64_t> GetInt(json::Object const& obj, const char *key) {
	if (auto i = Get<json::Integer>(obj, key)) return *i;
	if (auto d = Get<json::Double>(obj, key)) return static_cast<int64_t>(*d);
	return std::nullopt;
}

std::string GetString(json::Object const& obj, const char *key) {
	auto s = Get<json::String>(obj, key);
	return s ? *s : std::string();
}

template<typename T>
std::string ToJson(T const& value) {
	std::ostringstream ss;
	agi::JsonWriter::Write(value, ss);
	return ss.str();
}

json::Object TextBlock(std::string const& text) {
	json::Object block;
	block["type"] = "text";
	block["text"] = text;
	return block;
}

/// "3-5, 8, 10-12"
std::string FormatRanges(std::vector<int> rows) {
	std::sort(rows.begin(), rows.end());
	std::string out;
	for (size_t i = 0; i < rows.size(); ) {
		size_t j = i;
		while (j + 1 < rows.size() && rows[j + 1] == rows[j] + 1) ++j;
		if (!out.empty()) out += ", ";
		out += std::to_string(rows[i]);
		if (j > i) out += "-" + std::to_string(rows[j]);
		i = j + 1;
	}
	return out;
}

struct ToolCall {
	std::string id;
	std::string name;
	/// Serialized input, so it can be handed to the GUI thread
	std::string input;
};

struct ToolOutput {
	std::string text;
	/// Base64 JPEG, if the tool returns an image
	std::string image;
	bool is_error = false;

	static ToolOutput Ok(std::string text) { return {std::move(text), {}, false}; }
	static ToolOutput Error(std::string text) { return {std::move(text), {}, true}; }
};

struct HttpResult {
	long status = 0;
	std::string body;
	CURLcode curl_code = CURLE_OK;
	std::string curl_error;
	/// Seconds from the retry-after header, or -1
	int retry_after = -1;
};
}

namespace ai {

struct Agent::State {
	// GUI thread only
	agi::Context *c = nullptr;
	AgentListener *listener = nullptr;
	int frames_used = 0;
	int commit_id = -1;

	std::atomic<bool> running{false};
	std::atomic<bool> cancelled{false};
	/// The Agent was destroyed, so nothing should be posted to the GUI thread
	/// any more (the application may be shutting down)
	std::atomic<bool> destroyed{false};
	/// Incremented for each request so that callbacks from an abandoned
	/// request can be told apart from those of the current one
	std::atomic<int> generation{0};

	// Owned by the worker thread while running, and the GUI thread otherwise
	json::Array history;
	/// Which service's format the history is in
	Provider history_provider = Provider::Claude;
	AgentSettings settings;
	AgentUsage usage;
};

bool IsPlaceholder(agi::fs::path const& media) {
	auto name = media.string();
	return name.starts_with("?dummy") || name.starts_with("dummy-audio:");
}

namespace {
using State = std::shared_ptr<Agent::State>;

/// Run a function on the GUI thread on behalf of the listener, if this request
/// is still the current one
template<typename F>
void Notify(State const& s, int gen, F&& f) {
	if (s->destroyed) return;
	agi::dispatch::Main().Async([s, gen, f = std::forward<F>(f)] {
		if (s->listener && s->generation == gen)
			f(*s->listener);
	});
}

bool Live(State const& s, int gen) {
	return s->c && !s->cancelled && s->generation == gen;
}

// ---------------------------------------------------------------------------
// Tools. These all run on the GUI thread.

std::vector<AssDialogue *> AllLines(agi::Context *c) {
	std::vector<AssDialogue *> lines;
	for (auto& line : c->ass->Events)
		lines.push_back(&line);
	return lines;
}

ToolOutput GetProjectInfo(Agent::State& s) {
	auto c = s.c;
	auto lines = AllLines(c);
	int comments = std::count_if(lines.begin(), lines.end(), [](AssDialogue *d) { return d->Comment; });

	std::ostringstream out;
	auto title = c->ass->GetScriptInfo("Title");
	out << "Script title: " << (title.empty() ? "(none)" : std::string(title)) << "\n";
	int w = 0, h = 0;
	c->ass->GetResolution(w, h);
	out << "Script resolution: " << w << "x" << h << "\n";
	out << "Lines: " << lines.size() << " (" << comments << " of them comments), rows 1-" << lines.size() << "\n";

	out << "Styles:";
	for (auto const& style : c->ass->Styles)
		out << "\n  " << style.name << " (font " << style.font << ", size " << style.fontsize << ")";
	out << "\n";

	if (auto provider = c->project->VideoProvider(); provider && IsPlaceholder(c->project->VideoName()))
		out << "Video: a blank placeholder (dummy video), so there is nothing to see in it\n";
	else if (provider) {
		auto const& fps = c->project->Timecodes();
		int frames = provider->GetFrameCount();
		out << "Video: " << c->project->VideoName().filename().string() << ", "
		    << provider->GetWidth() << "x" << provider->GetHeight() << ", "
		    << frames << " frames";
		if (fps.IsLoaded() && frames > 0)
			out << ", " << agi::Time(fps.TimeAtFrame(frames - 1)).GetAssFormatted() << " long";
		out << "\n";
	}
	else
		out << "Video: none loaded, so get_video_frame is unavailable\n";

	auto const& audio = c->project->AudioName();
	out << "Audio: " << (audio.empty() ? std::string("none loaded")
		: IsPlaceholder(audio) ? std::string("a placeholder (dummy audio)")
		: audio.filename().string()) << "\n";

	std::map<AssDialogue *, int> rows;
	for (size_t i = 0; i < lines.size(); ++i) rows[lines[i]] = i + 1;
	std::vector<int> selected;
	for (auto line : c->selectionController->GetSelectedSet()) {
		auto it = rows.find(line);
		if (it != rows.end()) selected.push_back(it->second);
	}
	out << "Selected rows: " << (selected.empty() ? std::string("none") : FormatRanges(selected)) << "\n";
	if (auto active = c->selectionController->GetActiveLine()) {
		auto it = rows.find(active);
		if (it != rows.end()) out << "Active row: " << it->second << "\n";
	}

	int frames_left = c->project->VideoProvider() ? std::max(0, s.settings.max_frames - s.frames_used) : 0;
	out << "Video frames you may still look at during this request: " << frames_left << "\n";

	s.listener->OnAgentActivity(from_wx(_("Read the project information")));
	return ToolOutput::Ok(out.str());
}

ToolOutput ReadLines(Agent::State& s, json::Object const& input) {
	auto lines = AllLines(s.c);
	auto first = GetInt(input, "first_row");
	auto last = GetInt(input, "last_row");
	if (!first || !last)
		return ToolOutput::Error("first_row and last_row are required integers");
	if (lines.empty())
		return ToolOutput::Error("The file has no lines");

	int64_t from = std::max<int64_t>(1, *first);
	int64_t to = std::min<int64_t>(lines.size(), *last);
	if (from > to)
		return ToolOutput::Error(agi::format("No rows in that range; the file has rows 1-%d", (int)lines.size()));
	bool truncated = false;
	if (to - from + 1 > (int64_t)max_read_rows) {
		to = from + max_read_rows - 1;
		truncated = true;
	}

	std::ostringstream out;
	for (int64_t row = from; row <= to; ++row) {
		auto line = lines[row - 1];
		out << "#" << row << " " << line->Start.GetAssFormatted() << "-" << line->End.GetAssFormatted()
		    << " style=" << line->Style.get();
		if (!line->Actor.get().empty())
			out << " actor=" << line->Actor.get();
		if (line->Comment)
			out << " (comment)";
		out << ": " << line->Text.get() << "\n";
	}
	if (truncated)
		out << "(Stopped at row " << to << "; at most " << max_read_rows << " rows are returned per call.)\n";

	s.listener->OnAgentActivity(from_wx(fmt_tl("Read rows %d-%d", (int)from, (int)to)));
	return ToolOutput::Ok(out.str());
}

ToolOutput SetLineTexts(Agent::State& s, json::Object const& input) {
	auto edits = Get<json::Array>(input, "edits");
	if (!edits)
		return ToolOutput::Error("edits must be an array of {row, text} objects");
	if (edits->size() > max_edit_rows)
		return ToolOutput::Error(agi::format("At most %d lines can be changed per call; split the edits into smaller batches", (int)max_edit_rows));

	auto lines = AllLines(s.c);
	std::vector<int> changed;
	std::vector<std::string> errors;
	for (auto const& edit_elem : *edits) {
		json::Object const *edit = nullptr;
		try {
			edit = &static_cast<json::Object const&>(edit_elem);
		}
		catch (json::Exception const&) { }
		auto row = edit ? GetInt(*edit, "row") : std::nullopt;
		auto text = edit ? Get<json::String>(*edit, "text") : nullptr;
		if (!row || !text) {
			errors.push_back("an edit is missing its row or text");
			continue;
		}
		if (*row < 1 || *row > (int64_t)lines.size()) {
			errors.push_back(agi::format("row %d does not exist", (int)*row));
			continue;
		}

		// Real newlines aren't valid in ASS text; treat them as hard line breaks
		std::string new_text = *text;
		boost::replace_all(new_text, "\r\n", "\\N");
		boost::replace_all(new_text, "\n", "\\N");

		auto line = lines[*row - 1];
		if (line->Text.get() != new_text) {
			line->Text = new_text;
			changed.push_back(*row);
		}
	}

	if (!changed.empty()) {
		// Amend a single commit, so each request is one undo step
		s.commit_id = s.c->ass->Commit(_("AI translation"), AssFile::COMMIT_DIAG_TEXT, s.commit_id,
			changed.size() == 1 ? lines[changed[0] - 1] : nullptr);
		s.listener->OnAgentActivity(from_wx(fmt_plural(changed.size(), "Edited %d line (%s)", "Edited %d lines (%s)", (int)changed.size(), FormatRanges(changed))));
	}

	std::ostringstream out;
	out << "Updated " << changed.size() << " line(s)";
	if (!changed.empty()) out << ": rows " << FormatRanges(changed);
	out << ".";
	for (auto const& error : errors)
		out << "\nError: " << error << ".";
	if (changed.empty() && !errors.empty())
		return ToolOutput::Error(out.str());
	return ToolOutput::Ok(out.str());
}

ToolOutput GetVideoFrame(Agent::State& s, json::Object const& input) {
	auto c = s.c;
	auto provider = c->project->VideoProvider();
	if (!provider)
		return ToolOutput::Error("No video is loaded");
	if (s.frames_used >= s.settings.max_frames)
		return ToolOutput::Error(s.settings.max_frames == 0
			? "Looking at the video is turned off for this request"
			: "No video frames are left for this request; continue without the video");

	int time;
	std::string where;
	if (auto row = GetInt(input, "row")) {
		auto lines = AllLines(c);
		if (*row < 1 || *row > (int64_t)lines.size())
			return ToolOutput::Error(agi::format("row %d does not exist", (int)*row));
		auto line = lines[*row - 1];
		time = (static_cast<int>(line->Start) + static_cast<int>(line->End)) / 2;
		where = agi::format("row %d", (int)*row);
	}
	else if (auto ms = GetInt(input, "time_ms")) {
		time = static_cast<int>(*ms);
	}
	else
		time = c->project->Timecodes().TimeAtFrame(c->videoController->GetFrameN());

	auto const& fps = c->project->Timecodes();
	int frame = std::clamp(fps.FrameAtTime(time), 0, std::max(0, provider->GetFrameCount() - 1));
	time = fps.TimeAtFrame(frame);
	bool with_subs = false;
	if (auto b = Get<json::Boolean>(input, "with_subtitles")) with_subs = *b;

	wxImage image = GetImage(*provider->GetFrame(frame, time, !with_subs));
	if (!image.IsOk())
		return ToolOutput::Error("The frame could not be decoded");
	int w = image.GetWidth(), h = image.GetHeight();
	if (std::max(w, h) > frame_max_dimension) {
		double scale = double(frame_max_dimension) / std::max(w, h);
		image.Rescale(std::max(1, int(w * scale)), std::max(1, int(h * scale)), wxIMAGE_QUALITY_HIGH);
	}

	if (!wxImage::FindHandler(wxBITMAP_TYPE_JPEG))
		wxImage::AddHandler(new wxJPEGHandler);
	image.SetOption(wxIMAGE_OPTION_QUALITY, 80);
	wxMemoryOutputStream stream;
	if (!image.SaveFile(stream, wxBITMAP_TYPE_JPEG))
		return ToolOutput::Error("The frame could not be encoded");
	std::vector<char> data(stream.GetSize());
	stream.CopyTo(data.data(), data.size());

	++s.frames_used;
	auto timestamp = agi::Time(time).GetAssFormatted();
	s.listener->OnAgentActivity(from_wx(fmt_tl("Looked at the video at %s", timestamp)));

	ToolOutput out;
	out.text = "Frame " + std::to_string(frame) + " at " + timestamp;
	if (!where.empty()) out.text += " (middle of " + where + ")";
	out.text += with_subs ? ", with subtitles drawn." : ", without subtitles.";
	out.image = wxBase64Encode(data.data(), data.size()).ToStdString();
	return out;
}

ToolOutput ExecuteTool(Agent::State& s, ToolCall const& call) {
	json::UnknownElement root;
	try {
		std::istringstream ss(call.input);
		json::Reader::Read(root, ss);
	}
	catch (json::Exception const&) {
		return ToolOutput::Error("The tool input was not valid JSON");
	}
	json::Object const *input = nullptr;
	try {
		input = &static_cast<json::Object const&>(root);
	}
	catch (json::Exception const&) {
		return ToolOutput::Error("The tool input must be an object");
	}

	try {
		if (call.name == "get_project_info") return GetProjectInfo(s);
		if (call.name == "read_lines") return ReadLines(s, *input);
		if (call.name == "set_line_texts") return SetLineTexts(s, *input);
		if (call.name == "get_video_frame") return GetVideoFrame(s, *input);
	}
	catch (agi::Exception const& e) {
		return ToolOutput::Error(e.GetMessage());
	}
	catch (std::exception const& e) {
		return ToolOutput::Error(e.what());
	}
	return ToolOutput::Error("Unknown tool: " + call.name);
}

/// Run the tool calls on the GUI thread and wait for the results. Returns
/// nothing if the request was cancelled while waiting.
std::optional<std::vector<ToolOutput>> RunTools(State const& s, int gen, std::vector<ToolCall> calls) {
	if (s->destroyed) return std::nullopt;
	auto promise = std::make_shared<std::promise<std::vector<ToolOutput>>>();
	auto future = promise->get_future();
	agi::dispatch::Main().Async([s, gen, promise, calls = std::move(calls)] {
		std::vector<ToolOutput> results;
		for (auto const& call : calls) {
			if (!Live(s, gen) || !s->listener)
				results.push_back(ToolOutput::Error("Cancelled by the user"));
			else
				results.push_back(ExecuteTool(*s, call));
		}
		promise->set_value(std::move(results));
	});

	while (future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
		if (s->cancelled) return std::nullopt;
	}
	return future.get();
}

// ---------------------------------------------------------------------------
// HTTP

struct HttpRequest {
	std::string url;
	std::vector<std::string> headers;
	/// POSTed if not empty; otherwise the request is a GET
	std::string body;
	long timeout = 600;
};

size_t WriteBody(char *ptr, size_t size, size_t nmemb, void *userdata) {
	static_cast<std::string *>(userdata)->append(ptr, size * nmemb);
	return size * nmemb;
}

size_t ReadHeader(char *ptr, size_t size, size_t nmemb, void *userdata) {
	std::string header(ptr, size * nmemb);
	auto colon = header.find(':');
	if (colon != std::string::npos) {
		std::string name = boost::algorithm::to_lower_copy(header.substr(0, colon));
		if (name == "retry-after") {
			try {
				static_cast<HttpResult *>(userdata)->retry_after = std::stoi(header.substr(colon + 1));
			}
			catch (std::exception const&) { }
		}
	}
	return size * nmemb;
}

int Progress(void *userdata, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
	auto cancelled = static_cast<std::atomic<bool> const *>(userdata);
	return cancelled && *cancelled ? 1 : 0;
}

/// Perform a request, aborting early if *cancelled becomes true
HttpResult Fetch(HttpRequest const& request, std::atomic<bool> const *cancelled) {
	HttpResult result;
	CURL *curl = curl_easy_init();
	if (!curl) {
		result.curl_code = CURLE_FAILED_INIT;
		result.curl_error = "curl could not be initialized";
		return result;
	}

	curl_slist *headers = nullptr;
	if (!request.body.empty())
		headers = curl_slist_append(headers, "content-type: application/json");
	for (auto const& header : request.headers)
		headers = curl_slist_append(headers, header.c_str());

	char error_buffer[CURL_ERROR_SIZE] = {0};
	curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	if (!request.body.empty()) {
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.data());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
	}
	curl_easy_setopt(curl, CURLOPT_USERAGENT, agi::format("Aegisub %s", GetAegisubLongVersionString()).c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBody);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, ReadHeader);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, Progress);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, cancelled);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, std::min(request.timeout, 30L));
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, request.timeout);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

	result.curl_code = curl_easy_perform(curl);
	if (result.curl_code == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
	else
		result.curl_error = *error_buffer ? error_buffer : curl_easy_strerror(result.curl_code);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return result;
}

std::string JoinUrl(std::string base, const char *path) {
	while (!base.empty() && base.back() == '/')
		base.pop_back();
	return base + path;
}

bool IsRetryable(HttpResult const& r) {
	if (r.curl_code == CURLE_ABORTED_BY_CALLBACK) return false;
	if (r.curl_code != CURLE_OK) return true;
	return r.status == 408 || r.status == 409 || r.status == 429 || r.status >= 500;
}

/// The message from an error response ({"error": {"message": ...}} from the
/// Claude API, {"error": "..."} from Ollama), or the start of the body
std::string ErrorText(HttpResult const& r) {
	try {
		json::UnknownElement root;
		std::istringstream ss(r.body);
		json::Reader::Read(root, ss);
		json::Object const& obj = root;
		if (auto error = Get<json::Object>(obj, "error")) {
			auto message = GetString(*error, "message");
			if (!message.empty()) return message;
		}
		auto message = GetString(obj, "error");
		if (!message.empty()) return message;
	}
	catch (json::Exception const&) { }
	return r.body.substr(0, 300);
}

/// Sleep, waking early if the request is cancelled. Returns false if cancelled.
bool Wait(State const& s, int seconds) {
	for (int i = 0; i < seconds * 10; ++i) {
		if (s->cancelled) return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	return !s->cancelled;
}

bool HasRole(json::UnknownElement const& message, const char *role) {
	try {
		return GetString(static_cast<json::Object const&>(message), "role") == role;
	}
	catch (json::Exception const&) {
		return false;
	}
}

// ---------------------------------------------------------------------------
// Backends: the wire format of each service

enum class Stop {
	/// The model is done
	Done,
	/// The model called tools
	ToolUse,
	/// The output limit was reached
	MaxTokens,
	/// The model declined the request
	Refusal,
	/// Send the conversation again without adding anything
	Continue,
};

/// One response from the model
struct Turn {
	std::string text;
	std::vector<ToolCall> calls;
	Stop stop = Stop::Done;
	std::string refusal;
	int64_t input_tokens = 0;
	int64_t output_tokens = 0;
	int64_t cache_read_tokens = 0;
	int64_t cache_write_tokens = 0;
};

class Backend {
public:
	virtual ~Backend() = default;
	/// Add a message from the user to the end of the history
	virtual void AddUserMessage(json::Array& history, std::string const& text) const = 0;
	virtual HttpRequest Request(json::Array const& history, AgentSettings const& settings) const = 0;
	/// Read a successful response, adding the model's message to the history.
	/// Returns nothing if the response can't be understood.
	virtual std::optional<Turn> Parse(std::string const& body, json::Array& history) const = 0;
	/// Add the results of the tool calls in the last model message
	virtual void AddToolResults(json::Array& history, std::vector<ToolCall> const& calls, std::vector<ToolOutput> const& outputs) const = 0;
	virtual int MaxRetries() const = 0;
};

class ClaudeBackend final : public Backend {
	/// Tool calls in a message's content
	static std::vector<ToolCall> GetToolCalls(json::Object const& message) {
		std::vector<ToolCall> calls;
		auto content = Get<json::Array>(message, "content");
		if (!content) return calls;
		for (auto const& elem : *content) {
			try {
				json::Object const& block = elem;
				if (GetString(block, "type") != "tool_use") continue;
				ToolCall call;
				call.id = GetString(block, "id");
				call.name = GetString(block, "name");
				if (auto input = Get<json::Object>(block, "input"))
					call.input = ToJson(*input);
				else
					call.input = "{}";
				calls.push_back(std::move(call));
			}
			catch (json::Exception const&) { }
		}
		return calls;
	}

	static json::Object ToolResultBlock(std::string const& id, ToolOutput const& output) {
		json::Array content;
		content.push_back(TextBlock(output.text.empty() ? "Done" : output.text));
		if (!output.image.empty()) {
			json::Object source;
			source["type"] = "base64";
			source["media_type"] = "image/jpeg";
			source["data"] = output.image;
			json::Object image;
			image["type"] = "image";
			image["source"] = std::move(source);
			content.push_back(std::move(image));
		}

		json::Object block;
		block["type"] = "tool_result";
		block["tool_use_id"] = id;
		block["content"] = std::move(content);
		if (output.is_error)
			block["is_error"] = true;
		return block;
	}

	/// Move a response's content into an assistant message. After a server-side
	/// fallback partway through a response, the declined model's thinking and
	/// tool calls before the switch must not be sent back.
	static json::Object AssistantMessage(json::Array& content) {
		std::optional<size_t> last_fallback;
		for (size_t i = 0; i < content.size(); ++i) {
			try {
				if (GetString(static_cast<json::Object const&>(content[i]), "type") == "fallback")
					last_fallback = i;
			}
			catch (json::Exception const&) { }
		}

		json::Array kept;
		for (size_t i = 0; i < content.size(); ++i) {
			if (last_fallback && i < *last_fallback) {
				try {
					auto type = GetString(static_cast<json::Object const&>(content[i]), "type");
					if (type == "thinking" || type == "redacted_thinking" || type == "tool_use")
						continue;
				}
				catch (json::Exception const&) { }
			}
			kept.push_back(std::move(content[i]));
		}

		json::Object message;
		message["role"] = "assistant";
		message["content"] = std::move(kept);
		return message;
	}

public:
	/// If the conversation ended on unanswered tool calls (for example because
	/// the user stopped the agent), they're answered first so that the history
	/// stays valid. Consecutive user messages are merged.
	void AddUserMessage(json::Array& history, std::string const& text) const override {
		if (history.empty() || !HasRole(history.back(), "user")) {
			json::Array content;
			if (!history.empty()) {
				for (auto const& call : GetToolCalls(history.back()))
					content.push_back(ToolResultBlock(call.id, ToolOutput::Error("Cancelled by the user")));
			}
			json::Object message;
			message["role"] = "user";
			message["content"] = std::move(content);
			history.push_back(std::move(message));
		}
		json::Object& last = history.back();
		json::Array& content = last["content"];
		content.push_back(TextBlock(text));
	}

	HttpRequest Request(json::Array const& history, AgentSettings const& settings) const override {
		const char *env = std::getenv("ANTHROPIC_BASE_URL");
		HttpRequest request;
		request.url = JoinUrl(env && *env ? env : default_base_url, "/v1/messages");
		request.headers.push_back("anthropic-version: 2023-06-01");
		request.headers.push_back("x-api-key: " + settings.api_key);
		if (SupportsDefaultFallback(settings.model))
			request.headers.push_back(std::string("anthropic-beta: ") + fallback_beta);

		std::ostringstream body;
		body << "{\"model\":" << ToJson(settings.model)
		     << ",\"max_tokens\":" << max_output_tokens
		     << ",\"system\":" << ToJson(std::string(system_prompt))
		     << ",\"tools\":" << tools_json
		     // Cache the conversation so far, which is resent with every turn
		     << ",\"cache_control\":{\"type\":\"ephemeral\"}";
		if (!settings.effort.empty())
			body << ",\"output_config\":{\"effort\":" << ToJson(settings.effort) << "}";
		if (SupportsDefaultFallback(settings.model))
			body << ",\"fallbacks\":\"default\"";
		body << ",\"messages\":" << ToJson(history) << "}";
		request.body = body.str();
		return request;
	}

	std::optional<Turn> Parse(std::string const& body, json::Array& history) const override {
		json::UnknownElement root;
		json::Object *response = nullptr;
		json::Array *content = nullptr;
		try {
			std::istringstream ss(body);
			json::Reader::Read(root, ss);
			response = &static_cast<json::Object&>(root);
			content = &static_cast<json::Array&>((*response)["content"]);
		}
		catch (json::Exception const& e) {
			LOG_E("ai/agent") << "Could not parse response: " << e.what();
			return std::nullopt;
		}

		Turn turn;
		if (auto u = Get<json::Object>(*response, "usage")) {
			turn.input_tokens = GetInt(*u, "input_tokens").value_or(0);
			turn.output_tokens = GetInt(*u, "output_tokens").value_or(0);
			turn.cache_read_tokens = GetInt(*u, "cache_read_input_tokens").value_or(0);
			turn.cache_write_tokens = GetInt(*u, "cache_creation_input_tokens").value_or(0);
		}

		auto stop_reason = GetString(*response, "stop_reason");
		if (stop_reason == "refusal") {
			turn.stop = Stop::Refusal;
			if (auto details = Get<json::Object>(*response, "stop_details"))
				turn.refusal = GetString(*details, "explanation");
		}
		else if (stop_reason == "pause_turn")
			turn.stop = Stop::Continue;
		else if (stop_reason == "max_tokens")
			turn.stop = Stop::MaxTokens;

		// A refusal before any output has no content, and an empty assistant
		// message isn't valid in the history
		if (content->empty())
			return turn;

		history.push_back(AssistantMessage(*content));
		json::Object const& last = history.back();
		if (auto blocks = Get<json::Array>(last, "content")) {
			for (auto const& elem : *blocks) {
				try {
					json::Object const& block = elem;
					if (GetString(block, "type") == "text") {
						if (!turn.text.empty()) turn.text += "\n\n";
						turn.text += GetString(block, "text");
					}
				}
				catch (json::Exception const&) { }
			}
		}
		turn.calls = GetToolCalls(last);
		if (!turn.calls.empty() && turn.stop == Stop::Done)
			turn.stop = Stop::ToolUse;
		return turn;
	}

	void AddToolResults(json::Array& history, std::vector<ToolCall> const& calls, std::vector<ToolOutput> const& outputs) const override {
		json::Array results;
		for (size_t i = 0; i < calls.size(); ++i)
			results.push_back(ToolResultBlock(calls[i].id, outputs[i]));
		json::Object user;
		user["role"] = "user";
		user["content"] = std::move(results);
		history.push_back(std::move(user));
	}

	int MaxRetries() const override { return max_retries; }
};

/// Ollama's native chat API. Unlike the OpenAI-compatible endpoint, it lets
/// the request set the context window: Ollama's default is small, and it
/// silently drops the start of a prompt that doesn't fit.
class OllamaBackend final : public Backend {
	/// The tool definitions in Ollama's format, converted from the Claude ones
	static std::string const& Tools() {
		static const std::string tools = [] {
			json::UnknownElement root;
			std::istringstream ss(tools_json);
			json::Reader::Read(root, ss);
			json::Array converted;
			for (auto& elem : static_cast<json::Array&>(root)) {
				json::Object& tool = elem;
				json::Object function;
				function["name"] = std::move(tool["name"]);
				function["description"] = std::move(tool["description"]);
				function["parameters"] = std::move(tool["input_schema"]);
				json::Object wrapper;
				wrapper["type"] = "function";
				wrapper["function"] = std::move(function);
				converted.push_back(std::move(wrapper));
			}
			return ToJson(converted);
		}();
		return tools;
	}

	static std::vector<ToolCall> GetToolCalls(json::Object const& message) {
		std::vector<ToolCall> calls;
		auto tool_calls = Get<json::Array>(message, "tool_calls");
		if (!tool_calls) return calls;
		for (auto const& elem : *tool_calls) {
			try {
				json::Object const& call_obj = elem;
				auto function = Get<json::Object>(call_obj, "function");
				if (!function) continue;
				ToolCall call;
				call.id = GetString(call_obj, "id");
				call.name = GetString(*function, "name");
				if (auto input = Get<json::Object>(*function, "arguments"))
					call.input = ToJson(*input);
				else if (auto input = Get<json::String>(*function, "arguments"))
					call.input = *input; // Some models send the arguments as a JSON string
				else
					call.input = "{}";
				calls.push_back(std::move(call));
			}
			catch (json::Exception const&) { }
		}
		return calls;
	}

	static json::Object ToolMessage(ToolCall const& call, ToolOutput const& output) {
		json::Object message;
		message["role"] = "tool";
		message["tool_name"] = call.name;
		if (!call.id.empty())
			message["tool_call_id"] = call.id;
		message["content"] = (output.is_error ? "Error: " : "") + (output.text.empty() ? std::string("Done") : output.text);
		return message;
	}

public:
	void AddUserMessage(json::Array& history, std::string const& text) const override {
		if (!history.empty() && HasRole(history.back(), "assistant")) {
			json::Object const& last = history.back();
			auto calls = GetToolCalls(last);
			for (auto const& call : calls)
				history.push_back(ToolMessage(call, ToolOutput::Error("Cancelled by the user")));
		}
		json::Object message;
		message["role"] = "user";
		message["content"] = text;
		history.push_back(std::move(message));
	}

	HttpRequest Request(json::Array const& history, AgentSettings const& settings) const override {
		HttpRequest request;
		request.url = JoinUrl(settings.server_url, "/api/chat");
		// Local models can take a while, especially while loading
		request.timeout = 1800;

		json::Object system;
		system["role"] = "system";
		system["content"] = std::string(system_prompt);
		std::string messages = "[" + ToJson(system);
		for (auto const& message : history)
			messages += "," + ToJson(message);
		messages += "]";

		std::ostringstream body;
		body << "{\"model\":" << ToJson(settings.model)
		     << ",\"stream\":false"
		     << ",\"options\":{\"num_ctx\":" << std::max(2048, settings.context_length) << "}"
		     << ",\"tools\":" << Tools()
		     << ",\"messages\":" << messages << "}";
		request.body = body.str();
		return request;
	}

	std::optional<Turn> Parse(std::string const& body, json::Array& history) const override {
		json::UnknownElement root;
		json::Object *response = nullptr;
		json::Object *message = nullptr;
		try {
			std::istringstream ss(body);
			json::Reader::Read(root, ss);
			response = &static_cast<json::Object&>(root);
			message = &static_cast<json::Object&>((*response)["message"]);
		}
		catch (json::Exception const& e) {
			LOG_E("ai/agent") << "Could not parse response: " << e.what();
			return std::nullopt;
		}

		Turn turn;
		turn.input_tokens = GetInt(*response, "prompt_eval_count").value_or(0);
		turn.output_tokens = GetInt(*response, "eval_count").value_or(0);
		turn.text = GetString(*message, "content");
		boost::trim(turn.text);
		turn.calls = GetToolCalls(*message);
		// Tool calls don't always have IDs; they're only used to match results
		for (size_t i = 0; i < turn.calls.size(); ++i) {
			if (turn.calls[i].id.empty())
				turn.calls[i].id = "call_" + std::to_string(history.size()) + "_" + std::to_string(i);
		}

		if (!turn.calls.empty())
			turn.stop = Stop::ToolUse;
		else if (GetString(*response, "done_reason") == "length")
			turn.stop = Stop::MaxTokens;

		(*message)["role"] = "assistant";
		history.push_back(std::move(*message));
		return turn;
	}

	void AddToolResults(json::Array& history, std::vector<ToolCall> const& calls, std::vector<ToolOutput> const& outputs) const override {
		json::Array images;
		std::string image_notes;
		for (size_t i = 0; i < calls.size(); ++i) {
			history.push_back(ToolMessage(calls[i], outputs[i]));
			if (!outputs[i].image.empty()) {
				images.push_back(outputs[i].image);
				image_notes += "\n" + outputs[i].text;
			}
		}
		// Images can't go in tool messages, so they follow in a user message
		if (!images.empty()) {
			json::Object message;
			message["role"] = "user";
			message["content"] = "The video frames you asked for:" + image_notes;
			message["images"] = std::move(images);
			history.push_back(std::move(message));
		}
	}

	// Errors from a local server (it isn't running, the model doesn't fit in
	// memory...) don't go away by waiting
	int MaxRetries() const override { return 0; }
};

Backend const& BackendFor(Provider provider) {
	static const ClaudeBackend claude;
	static const OllamaBackend ollama;
	return provider == Provider::Ollama ? static_cast<Backend const&>(ollama) : claude;
}

const char *ServiceName(Provider provider) {
	return provider == Provider::Ollama ? "Ollama" : "Claude API";
}

struct ModelInfo {
	bool tools = true;
	bool vision = true;
};

/// Ask Ollama what the model can do. Returns an error message on failure.
std::optional<std::string> CheckOllamaModel(State const& s, ModelInfo& info) {
	HttpRequest request;
	request.url = JoinUrl(s->settings.server_url, "/api/show");
	request.timeout = 30;
	json::Object body;
	body["model"] = s->settings.model;
	request.body = ToJson(body);

	auto result = Fetch(request, &s->cancelled);
	if (s->cancelled) return std::string();
	auto model = s->settings.model;
	auto url = s->settings.server_url;
	if (result.curl_code != CURLE_OK) {
		auto error = result.curl_error;
		LOG_E("ai/agent") << "Could not reach Ollama: " << error;
		return from_wx(fmt_tl("Could not connect to Ollama at %s. Make sure Ollama is running.\n(%s)", url, error));
	}
	if (result.status == 404)
		return from_wx(fmt_tl("The Ollama model \"%s\" isn't installed. Install it with: ollama pull %s", model, model));
	if (result.status != 200)
		return from_wx(fmt_tl("%s returned an error (%d): %s", "Ollama", (int)result.status, ErrorText(result)));

	try {
		json::UnknownElement root;
		std::istringstream ss(result.body);
		json::Reader::Read(root, ss);
		json::Object const& obj = root;
		// Older versions of Ollama don't report capabilities; assume the best
		if (auto caps = Get<json::Array>(obj, "capabilities")) {
			info.tools = info.vision = false;
			for (auto const& cap : *caps) {
				try {
					auto const& name = static_cast<json::String const&>(cap);
					if (name == "tools") info.tools = true;
					if (name == "vision") info.vision = true;
				}
				catch (json::Exception const&) { }
			}
		}
	}
	catch (json::Exception const&) { }

	if (!info.tools)
		return from_wx(fmt_tl("The Ollama model \"%s\" can't use tools, which the agent needs. Choose a model that supports tools, such as qwen3.", model));
	return std::nullopt;
}

// ---------------------------------------------------------------------------
// The agent loop. This runs on a worker thread.

/// Finish the current request. The error is built on the GUI thread so that it
/// can be translated.
template<typename F>
void Finish(State const& s, int gen, F&& make_error) {
	s->running = false;
	Notify(s, gen, [make_error = std::forward<F>(make_error)](AgentListener& l) {
		l.OnAgentFinished(make_error());
	});
}

void Succeed(State const& s, int gen) {
	Finish(s, gen, [] { return std::string(); });
}

void Stopped(State const& s, int gen) {
	Finish(s, gen, [] { return from_wx(_("Stopped.")); });
}

void Run(State s, int gen, std::string message) {
	auto provider = s->settings.provider;
	auto const& backend = BackendFor(provider);

	if (provider == Provider::Ollama) {
		ModelInfo info;
		auto error = CheckOllamaModel(s, info);
		if (s->cancelled) return Stopped(s, gen);
		if (error) return Finish(s, gen, [error] { return *error; });
		if (!info.vision && s->settings.max_frames > 0) {
			// Read on the GUI thread by the tools, which can't run before this
			// is posted
			s->settings.max_frames = 0;
			Notify(s, gen, [](AgentListener& l) {
				l.OnAgentActivity(from_wx(_("This model can't see images, so the video won't be used.")));
			});
		}
	}

	backend.AddUserMessage(s->history, message);

	for (int request = 0; request < max_requests; ++request) {
		if (s->cancelled) return Stopped(s, gen);

		auto http = backend.Request(s->history, s->settings);
		HttpResult result;
		for (int attempt = 0; ; ++attempt) {
			result = Fetch(http, &s->cancelled);
			if (s->cancelled) return Stopped(s, gen);
			if ((result.curl_code == CURLE_OK && result.status == 200) || !IsRetryable(result) || attempt >= backend.MaxRetries())
				break;

			int delay = result.retry_after >= 0 ? std::min(result.retry_after, 60) : 2 << attempt;
			LOG_D("ai/agent") << "Request failed (" << result.status << " " << result.curl_error << "), retrying in " << delay << "s";
			Notify(s, gen, [delay](AgentListener& l) {
				l.OnAgentActivity(from_wx(fmt_plural(delay, "The service is busy or unreachable; retrying in %d second...", "The service is busy or unreachable; retrying in %d seconds...", delay)));
			});
			if (!Wait(s, delay)) return Stopped(s, gen);
		}

		if (result.curl_code != CURLE_OK) {
			auto error = result.curl_error;
			auto url = s->settings.server_url;
			return Finish(s, gen, [provider, error, url] {
				if (provider == Provider::Ollama)
					return from_wx(fmt_tl("Could not connect to Ollama at %s. Make sure Ollama is running.\n(%s)", url, error));
				return from_wx(fmt_tl("Could not reach %s: %s", ServiceName(provider), error));
			});
		}
		if (result.status != 200) {
			auto status = result.status;
			auto message = ErrorText(result);
			LOG_E("ai/agent") << "API error " << status << ": " << message;
			return Finish(s, gen, [provider, status, message] {
				if (provider == Provider::Claude && status == 401)
					return from_wx(_("The Claude API key was rejected. Check it in Preferences > Automation."));
				return from_wx(fmt_tl("%s returned an error (%d): %s", ServiceName(provider), (int)status, message));
			});
		}

		auto turn = backend.Parse(result.body, s->history);
		if (!turn)
			return Finish(s, gen, [provider] { return from_wx(fmt_tl("The response from %s could not be read.", ServiceName(provider))); });

		s->usage.input_tokens += turn->input_tokens;
		s->usage.output_tokens += turn->output_tokens;
		s->usage.cache_read_tokens += turn->cache_read_tokens;
		s->usage.cache_write_tokens += turn->cache_write_tokens;
		++s->usage.requests;
		Notify(s, gen, [usage = s->usage](AgentListener& l) { l.OnAgentUsage(usage); });

		if (!turn->text.empty())
			Notify(s, gen, [text = turn->text](AgentListener& l) { l.OnAgentText(text); });

		switch (turn->stop) {
			case Stop::Refusal:
				return Finish(s, gen, [refusal = turn->refusal] {
					if (refusal.empty())
						return from_wx(_("Claude declined this request."));
					return from_wx(fmt_tl("Claude declined this request: %s", refusal));
				});
			case Stop::Continue:
				continue;
			case Stop::Done:
				return Succeed(s, gen);
			case Stop::MaxTokens:
				if (turn->calls.empty())
					return Finish(s, gen, [] { return from_wx(_("The response was cut off because it was too long. Ask for a smaller part of the file at a time.")); });
				break;
			case Stop::ToolUse:
				break;
		}

		std::vector<ToolOutput> outputs;
		if (turn->stop == Stop::MaxTokens) {
			// The tool input may have been cut off, so don't run it
			for (size_t i = 0; i < turn->calls.size(); ++i)
				outputs.push_back(ToolOutput::Error("Your response was cut off by the output limit before this tool call was complete. Make the call again with less input, for example fewer edits per call."));
		}
		else {
			auto ran = RunTools(s, gen, turn->calls);
			if (!ran) return Stopped(s, gen);
			outputs = std::move(*ran);
		}
		backend.AddToolResults(s->history, turn->calls, outputs);
	}

	Finish(s, gen, [] { return from_wx(_("The agent stopped after too many steps. Send a message to let it continue.")); });
}
}

Agent::Agent(agi::Context *c, AgentListener *listener)
: state(std::make_shared<State>())
{
	state->c = c;
	state->listener = listener;
	static bool curl_initialized = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
	(void)curl_initialized;
}

Agent::~Agent() {
	state->destroyed = true;
	state->listener = nullptr;
	state->c = nullptr;
	state->cancelled = true;
}

void Agent::Send(std::string const& message, AgentSettings const& settings) {
	if (state->running) return;
	state->running = true;
	state->cancelled = false;
	int gen = ++state->generation;
	state->settings = settings;
	// Each service has its own message format, so a conversation can't move
	// from one to another
	if (settings.provider != state->history_provider) {
		state->history.clear();
		state->history_provider = settings.provider;
	}
	state->frames_used = 0;
	state->commit_id = -1;
	std::thread(Run, state, gen, message).detach();
}

void Agent::Cancel() {
	state->cancelled = true;
}

void Agent::Reset() {
	if (state->running) return;
	state->history.clear();
	state->usage = AgentUsage();
}

bool Agent::IsRunning() const {
	return state->running;
}

std::vector<std::string> ListOllamaModels(std::string const& server_url, std::string& error) {
	std::vector<std::string> models;
	HttpRequest request;
	request.url = JoinUrl(server_url, "/api/tags");
	request.timeout = 5;
	auto result = Fetch(request, nullptr);
	if (result.curl_code != CURLE_OK) {
		error = result.curl_error;
		return models;
	}
	if (result.status != 200) {
		error = ErrorText(result);
		return models;
	}
	try {
		json::UnknownElement root;
		std::istringstream ss(result.body);
		json::Reader::Read(root, ss);
		json::Object const& obj = root;
		if (auto list = Get<json::Array>(obj, "models")) {
			for (auto const& elem : *list) {
				try {
					auto name = GetString(static_cast<json::Object const&>(elem), "name");
					if (!name.empty()) models.push_back(name);
				}
				catch (json::Exception const&) { }
			}
		}
	}
	catch (json::Exception const&) {
		error = "unexpected response";
	}
	std::sort(models.begin(), models.end());
	return models;
}

std::string GetApiKey() {
	std::string key = OPT_GET("Automation/AI Agent/API Key")->GetString();
	boost::trim(key);
	if (key.empty()) {
		if (const char *env = std::getenv("ANTHROPIC_API_KEY"))
			key = env;
		boost::trim(key);
	}
	return key;
}

}
