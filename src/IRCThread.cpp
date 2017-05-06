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

#include <cstring>
#include "IRCThread.h"
#include "CommandHandler.h"


std::string IRCThread::s_bot_name = "mybot_new";
irc_info_session IRCThread::s_iis = irc_info_session("server", "nick");
IRCThread *IRCThread::that = nullptr;

IRCThread::IRCThread(const std::string channel, const std::string nick)
{
	s_iis.channel = channel;
	s_iis.nick = nick;
	that = this;
}

IRCThread::~IRCThread()
{
	delete that;
}
void IRCThread::run(const char *server, unsigned short port)
{
	irc_callbacks_t callbacks = { 0 };
	irc_session_t *s;

	callbacks.event_connect = &IRCThread::event_connect;
	callbacks.event_join = &IRCThread::event_join;
	callbacks.event_channel = &IRCThread::event_channel;

	//std::thread co(IRCThread::connect(callbacks, server, port), this);
	// Ne sert à rien
	//std::thread thread_connect([this, callbacks, server, port] { this->connect(callbacks, server, port); });
	connect(callbacks, server, port);
}

void IRCThread::connect(irc_callbacks_t callbacks, const char *server, unsigned short port)
{
	std::cout << "Debut du thread connexion" << std::endl;
	while (m_run) {
		m_irc_session = irc_create_session(&callbacks);

		if (!m_irc_session) {
			std::cout << "Could not create session" << std::endl;
			return;
		}

		irc_set_ctx(m_irc_session, &s_iis);

		if (server[0] == '#' && server[1] == '#') {
			server++;

			irc_option_set(m_irc_session, LIBIRC_OPTION_SSL_NO_VERIFY);
		}

		std::cout << "Connection wait...";
		// Initiate the IRC server connection
		if (irc_connect(m_irc_session, server, port, 0, s_iis.nick.c_str(), 0, 0)) {
			std::cout << std::endl << "Could not connect " << irc_strerror(irc_errno(m_irc_session)) << std::endl;
			return;
		}

		std::cout << "..." << std::endl;

		if (irc_run(m_irc_session)) {
			std::cout << "Could not connect or I/O error: " << irc_strerror(irc_errno(m_irc_session)) << std::endl;
			return;
		}

		std::cout << "Connection done !" << std::endl;
	}
}

void IRCThread::event_connect(irc_session_t *session, const char *event, const char *origin,
			const char **params, unsigned int count)
{
	if (!irc_is_connected(session)) {
		std::cout << "Not connected to IRC" << std::endl;
	}
	std::cout << "Connected to IRC" << std::endl;

	s_bot_name = std::string(params[0]);

	if (irc_cmd_join(session, s_iis.channel.c_str(), NULL)) {
		std::cerr << "Unable to join channel " << s_iis.channel << ", aborting." << std::endl;
		irc_disconnect(session);
	}

}

void IRCThread::event_join(irc_session_t *session, const char *event, const char *origin,
			const char **params, unsigned int count)
{
	if (!irc_is_connected(session)) {
		std::cerr << "Error: not connected to IRC on " << __FUNCTION__ << std::endl;
		return;
	}

	std::cout << "Join channel" << std::endl;
	std::string msg = "";

	if (strcmp(origin, s_bot_name.c_str()) == 0)
		msg = "Salut " + std::string(origin) + " ! Tu vas bien ?";
	else
		msg = "Salut " + s_iis.channel + " ! Vous allez bien ? ";

	irc_cmd_msg(session, s_iis.channel.c_str(), msg.c_str());
}

void IRCThread::event_channel(irc_session_t *session, const char *event, const char *origin,
			const char **params, unsigned int count)
{
	if (!irc_is_connected(session)) {
		std::cerr << "Error: not connected to IRC on " << __FUNCTION__ << std::endl;
		return;
	}

	if (strcmp(origin, IRCThread::s_bot_name.c_str()) == 0 || count == 1) {
		return;
	}

	std::cout << "Event channel : " << params[0] << " : " << params[1] << std::endl;

	if (params[1][0] == '.') {
		CommandHandler *command_handler = new CommandHandler(that);
		std::string msg = "";
		bool res = command_handler->handle_command(params[1], msg, Permission::USER);

		that->add_text(msg);
	}
}

void IRCThread::add_text(const std::string &text)
{
	irc_cmd_msg(m_irc_session, s_iis.channel.c_str(), text.c_str());
}

void IRCThread::event_numeric(irc_session_t *session, const char *event, const char *origin,
			const char **params, unsigned int count)
{

}