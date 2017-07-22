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

#include <libircclient.h>
#include <libirc_events.h>
#include <iostream>

class Config;

struct irc_info_session {
	std::string channel;
	std::string nick;

	irc_info_session(std::string channel, std::string nick) :
			channel(channel), nick(nick) {};
};

class IRCThread {
public:
	IRCThread(const Config *cfg);
	~IRCThread();
	void run(const Config *cfg);
	void connect(irc_callbacks_t callbacks, const char *server, unsigned short port);

	void add_text(const std::string &text);
	void stop();

private:
	static void event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
	static void event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
	static void event_numeric(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
	static void event_channel(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
	static void event_privmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);

	static const Config *s_cfg;
	static irc_info_session s_iis;
	bool m_run = true;
	irc_session_t *m_irc_session = nullptr;
	static std::string s_bot_name;
	static IRCThread *that;
};

