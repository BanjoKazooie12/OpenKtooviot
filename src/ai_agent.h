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

/// @file ai_agent.h
/// @brief Agent which edits the open subtitles through the Claude API
/// @ingroup ai

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace agi { struct Context; }
namespace agi::fs { class path; }

namespace ai {

/// The service which runs the model
enum class Provider {
	/// Anthropic's Claude API
	Claude,
	/// A local Ollama server
	Ollama,
};

struct AgentSettings {
	Provider provider = Provider::Claude;
	std::string model;
	/// Claude only
	std::string api_key;
	/// Claude only; empty to use the model's default
	std::string effort;
	/// Ollama only: the server's address, e.g. http://localhost:11434
	std::string server_url;
	/// Ollama only: the context window to ask for, in tokens
	int context_length = 16384;
	/// Maximum number of video frames the agent may look at per request
	int max_frames = 0;
};

struct AgentUsage {
	int64_t input_tokens = 0;
	int64_t output_tokens = 0;
	int64_t cache_read_tokens = 0;
	int64_t cache_write_tokens = 0;
	int requests = 0;
};

/// Receives agent progress. All methods are called on the GUI thread.
class AgentListener {
public:
	virtual ~AgentListener() = default;
	/// Text written by the model
	virtual void OnAgentText(std::string const& text) = 0;
	/// Something the agent did, such as reading lines or editing them
	virtual void OnAgentActivity(std::string const& text) = 0;
	/// Cumulative usage for this conversation
	virtual void OnAgentUsage(AgentUsage const& usage) = 0;
	/// The request finished; error is empty on success
	virtual void OnAgentFinished(std::string const& error) = 0;
};

/// A conversation with the model about the subtitles open in a context.
///
/// Requests run on a background thread. Tools which touch the subtitles or the
/// video run on the GUI thread. The agent must be created and destroyed on the
/// GUI thread, and destroying it cancels any request in progress.
class Agent {
public:
	/// Shared with the worker thread; defined in ai_agent.cpp
	struct State;

private:
	std::shared_ptr<State> state;

public:
	Agent(agi::Context *c, AgentListener *listener);
	~Agent();

	Agent(Agent const&) = delete;
	Agent& operator=(Agent const&) = delete;

	/// Send a message and let the agent work until it's done. Does nothing if
	/// a request is already running.
	void Send(std::string const& message, AgentSettings const& settings);

	/// Stop the current request as soon as possible
	void Cancel();

	/// Forget the conversation so far. Does nothing while a request is running.
	void Reset();

	bool IsRunning() const;
};

/// The API key from the options, or from ANTHROPIC_API_KEY if that's empty
std::string GetApiKey();

/// The models installed on an Ollama server. Blocks; on failure returns an
/// empty list and sets error.
std::vector<std::string> ListOllamaModels(std::string const& server_url, std::string& error);

/// Is this the name of Aegisub's dummy video or audio?
bool IsPlaceholder(agi::fs::path const& media);

}
