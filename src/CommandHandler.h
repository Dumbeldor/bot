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
#include <iostream>
#include <core/utils/threads.h>

class IRCThread;
class CommandHandler;
class Config;

namespace winterwind
{
	namespace extras
	{
		class GitlabAPIClient;
	}
}

enum Permission : uint8_t
{
	USER,
	ADMIN,
	CONSOLE,
};

struct ChatCommand
{
	const char *name;
	bool (CommandHandler::*Handler)(const std::string &args, std::string &msg, const Permission &permission);
	ChatCommand *childCommand;
	const std::string help;
};

enum ChatCommandSearchResult : uint8_t
{
	CHAT_COMMAND_OK,
	CHAT_COMMAND_UNKNOWN,
	CHAT_COMMAND_UNKNOWN_SUBCOMMAND,
};

class CommandHandler: public Thread
{
public:
	CommandHandler(IRCThread *irc_thread, const Config *cfg, const std::string &text, const Permission &permission);
	~CommandHandler() {};

	void *run();

	bool handle_command(std::string &msg);

public:
	ChatCommandSearchResult find_command(ChatCommand *table, const char *&text,
										ChatCommand *&command, ChatCommand **parentCommand = nullptr);
	ChatCommand *getCommandTable();
	bool is_permission(const Permission &permission_required, const Permission &permission, std::string &msg) const;

	bool handle_command_list(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_help(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_weather(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_say(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_stop(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_vdm(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_chuck_norris(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_joke(const std::string &args, std::string &msg, const Permission &permission);
	bool handle_command_quote(const std::string &args, std::string &msg, const Permission &permission);

	bool handle_command_gitlab_issue(const std::string &args, std::string &msg, const Permission &permission);
	uint32_t get_gitlab_project_id(const std::string &project,
			const  std::string &ns, winterwind::extras::GitlabAPIClient &gitlab_client);

	IRCThread *m_irc_thread = nullptr;
	const std::string m_text;
	const Permission &m_permission;
	const Config *m_cfg = nullptr;
};

