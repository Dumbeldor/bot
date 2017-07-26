/**
 * Copyright (c) 2017, Vincent Glize <vincent.glize@live.fr>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <core/utils/threads.h>
#include <extras/ircclient.h>
#include <functional>
#include <unordered_set>
#include <queue>
#include <unordered_map>

typedef std::function<void (irc_session_t *session, const char *, const char *origin,
							const char ** params, unsigned int count)> irc_callback_f;

struct IRCChannel {
	std::string name = "";
	std::vector<std::string> members = {};
	std::string topic = "";
};

class IRCThread: public Thread, public IRCClient
{
public:
	IRCThread();
	~IRCThread();

	void *run();

	const std::string &get_name()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_name;
	}

	const std::unordered_map<std::string, IRCChannel *> &get_channels()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_channels;
	}

	void on_event_connect(const std::string &origin,
						  const std::vector<std::string> &params);
	void on_event_join(const std::string &origin,
					   const std::vector<std::string> &params);
	void on_event_part(const std::string &origin,
					   const std::vector<std::string> &params);
	void on_event_notice(const std::string &origin,
						 const std::vector<std::string> &params);
	void on_event_message(const std::string &origin,
						  const std::vector<std::string> &params);
	void on_event_kick(const std::string &origin,
					   const std::vector<std::string> &params);
	void on_event_topic(const std::string &origin,
						const std::vector<std::string> &params);

	void on_event_numeric(uint32_t event_id, const std::string &origin,
						  const std::vector<std::string> &params);

private:
	void set_name(const std::string &name)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_name = name;
	}

	void register_channel(const std::string &channel_name);
	void on_channel_leave(const std::string &channel_name);
	void set_channel_topic(const std::string &channel_name, const std::string &topic);

	void load_state();
	void save_state() const;

	std::string m_name = "";
	std::mutex m_mutex;

	std::unordered_map<std::string, IRCChannel *> m_channels;
	std::unordered_map<std::string, std::queue<std::string>> m_names_queues;
};


