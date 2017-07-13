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
#include <thread>
#include <cmath>
#include "CommandHandler.h"
#include "IRCThread.h"
#include "HttpClient.h"
#include "Console.h"
#include "Config.h"
#include "Mail.h"
#include <extras/gitlabapiclient.h>
#include <sstream>

using namespace winterwind::extras;

static const ChatCommand COMMANDHANDLERFINISHER = {nullptr, nullptr, nullptr, ""};

CommandHandler::CommandHandler(IRCThread *irc_thread, const Config *cfg, const std::string &text, const Permission &permission) : Thread(),
		m_irc_thread(irc_thread), m_cfg(cfg), m_text(text), m_permission(permission)
{

}

void* CommandHandler::run()
{
	Thread::set_thread_name("CommandHandler");

	ThreadStarted();

	std::string msg;
	handle_command(msg);

	return nullptr;
}

ChatCommand *CommandHandler::getCommandTable()
{
	static ChatCommand gitlabCommandTable[] {
			{"issue", &CommandHandler::handle_command_gitlab_issue, nullptr, "Usage: .gitlab issue <issue_id>"},
			COMMANDHANDLERFINISHER,
	};
	static ChatCommand globalCommandTable[] = {
			{"weather", &CommandHandler::handle_command_weather, nullptr, "Usage: .weather <ville>"},
			{"gitlab", nullptr, gitlabCommandTable, "Usage: .gitlab <issue>" },
			{"chuck_norris", &CommandHandler::handle_command_chuck_norris, nullptr, "Usage: .chuck_norris"},
			{"joke", &CommandHandler::handle_command_joke, nullptr, "Usage: .joke"},
			{"vdm", &CommandHandler::handle_command_vdm, nullptr, "Usage: .vdm"},
			{"quote", &CommandHandler::handle_command_quote, nullptr, "Usage: .quote"},
			{"say", &CommandHandler::handle_command_say, nullptr, "Usage: .say text"},
			{"help", &CommandHandler::handle_command_help, nullptr, ""},
			{"list", &CommandHandler::handle_command_list, nullptr, ""},
			{"mail", &CommandHandler::handle_command_mail, nullptr, "Usage: .mail <pseudo> <message>"},
			{"stop", &CommandHandler::handle_command_stop, nullptr, "Stop bot"},
			COMMANDHANDLERFINISHER,
	};

	return globalCommandTable;
}

bool CommandHandler::is_permission(const Permission &permission_required, const Permission &permission, std::string &msg) const
{
	if (permission_required > permission) {
		msg = "Tu n'as pas la permission !";
		return false;
	}
	return true;
}

bool CommandHandler::handle_command(std::string &msg)
{
	ChatCommand *command = nullptr;
	ChatCommand *parentCommand = nullptr;

	const char *ctext = &(m_text.c_str())[1];

	ChatCommandSearchResult res = find_command(getCommandTable(), ctext, command, &parentCommand);
	switch (res) {
		case CHAT_COMMAND_OK:
			(this->*(command->Handler))(ctext, msg, m_permission);
			break;
		case CHAT_COMMAND_UNKNOWN_SUBCOMMAND:
			msg = command->help;
			break;
		case CHAT_COMMAND_UNKNOWN:
			msg = "Unknown command.";
			break;
	}

	m_irc_thread->add_text(msg);
	stop();
}

ChatCommandSearchResult CommandHandler::find_command(ChatCommand *table, const char *&text, ChatCommand *&command,
													 ChatCommand **parentCommand)
{
	std::string cmd = "";

	// Skip whitespaces
	while (*text != ' ' && *text != '\0') {
		cmd += *text;
		++text;
	}

	while (*text == ' ') {
		++text;
	}

	for (int32_t i = 0; table[i].name != nullptr; ++i) {
		size_t len = strlen(table[i].name);
		if (strncmp(table[i].name, cmd.c_str(), len + 1) != 0) {
			continue;
		}

		if (table[i].childCommand != nullptr) {
			const char *stext = text;
			ChatCommand *parentSubCommand = nullptr;
			ChatCommandSearchResult res = find_command(
					table[i].childCommand, text, command, &parentSubCommand
			);

			switch (res) {
				case CHAT_COMMAND_OK:
					if (parentCommand) {https://github.com/yamashi/deep-learning-tuto
						*parentCommand = parentSubCommand ? parentSubCommand : &table[i];
					}

					if (strlen(command->name) == 0 && !parentSubCommand) {
						text = stext;
					}
					return CHAT_COMMAND_OK;

				case CHAT_COMMAND_UNKNOWN:
					command = &table[i];
					if (parentCommand)
						*parentCommand = nullptr;

					text = stext;
					return CHAT_COMMAND_UNKNOWN_SUBCOMMAND;

				case CHAT_COMMAND_UNKNOWN_SUBCOMMAND:
				default:
					if (parentCommand)
						*parentCommand = parentSubCommand ? parentSubCommand : &table[i];

					return res;
			}
		}

		if (!table[i].Handler) {
			continue;
		}

		command = &table[i];

		if (parentCommand) {
			*parentCommand = nullptr;
		}

		return CHAT_COMMAND_OK;
	}

	command = nullptr;

	if (parentCommand)
		*parentCommand = nullptr;

	return CHAT_COMMAND_UNKNOWN;
}

bool CommandHandler::handle_command_list(const std::string &args, std::string &msg, const Permission &permission)
{
	ChatCommand *cmds = getCommandTable();
	msg += "Command list : ";

	for (int32_t i = 0; cmds[i].name != nullptr; i++) {
		msg += std::string(cmds[i].name) + ", ";
	}
	return true;
}

bool CommandHandler::handle_command_help(const std::string &args, std::string &msg, const Permission &permission)
{
	if (args.empty()) {
		msg = "/help <command> to get the help of the command \n";
		return handle_command_list(args, msg, permission);
	}

	ChatCommand *command = nullptr;
	ChatCommand *parentCommand = nullptr;
	const char *ctext = args.c_str();
	msg = args;

	ChatCommandSearchResult res = find_command(
			getCommandTable(), ctext, command, &parentCommand
	);

	switch(res) {
		case CHAT_COMMAND_OK:
			msg = command->help;
			return true;

		case CHAT_COMMAND_UNKNOWN_SUBCOMMAND: {
			msg = command->help + "\n";
			ChatCommand *child = command->childCommand;
			for (uint16_t i = 0; child[i].name != nullptr; ++i) {
				msg += (std::string) child[i].name + "\n";
				msg += "		" + child[i].help + "\n";
			}
			return true;
		}

		case CHAT_COMMAND_UNKNOWN:
			msg = "Command not found";
			return false;
	}
}

bool CommandHandler::handle_command_weather(const std::string &args, std::string &msg, const Permission &permission)
{
	if (m_cfg->get_openweathermap_api_key() == "") {
		std::cerr << "Key openweather doesn't exist !" << std::endl;
		msg = "Key openweather doesn't exist !";
		return false;
	}
	HttpClient *http_server = new HttpClient();
	const std::string url = "http://api.openweathermap.org/data/2.5/weather?q="+args+"s&APPID="+m_cfg->get_openweathermap_api_key();
	Json::Value json_value;
	std::thread http([http_server, url, &json_value] { http_server->get_json(json_value, url); });
	while (http_server->is_running()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	http.detach();
	int temp = json_value["main"]["temp"].asDouble() - 273.15;
	int max = json_value["main"]["temp_max"].asDouble() - 273.15;
	int min = json_value["main"]["temp_min"].asDouble() - 273.15;
	msg = "La température  à " + json_value["name"].asString() + " est de " + std::to_string(temp) + " degrès. (min : " +
			std::to_string(min) + " max : " +
			std::to_string(max) + ")";
	return true;
}

bool CommandHandler::handle_command_say(const std::string &args, std::string &msg, const Permission &permission)
{
	if (is_permission(Permission::ADMIN, permission, msg)) {
		m_irc_thread->add_text(args);
	}

	return true;
}

bool CommandHandler::handle_command_stop(const std::string &args, std::string &msg, const Permission &permission)
{
	if (is_permission(Permission::ADMIN, permission, msg)) {
		msg = "Server stop...";
		m_irc_thread->add_text("Noooo, I died !! Good bye my friends !");
		Console::stop();
	}
	return true;
}

bool CommandHandler::handle_command_vdm(const std::string &args, std::string &msg, const Permission &permission)
{
	msg = "WIP";
	return true;
}

bool CommandHandler::handle_command_chuck_norris(const std::string &args, std::string &msg,
												 const Permission &permission)
{
	HttpClient *http_server = new HttpClient();
	const std::string url = "http://api.icndb.com/jokes/random";
	Json::Value json_value;
	std::thread http([http_server, url, &json_value] { http_server->get_json(json_value, url); });
	while (http_server->is_running()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	http.detach();
	msg = json_value["value"]["joke"].asString();
	return true;
}

bool CommandHandler::handle_command_joke(const std::string &args, std::string &msg,
												 const Permission &permission)
{
	HttpClient *http_server = new HttpClient();
	const std::string url = "http://webknox.com/api/jokes/random?apiKey=bejebgdahjzmcxjyxbkpmbmbvtttidu";
	Json::Value json_value;
	std::thread http([http_server, url, &json_value] { http_server->get_json(json_value, url); });
	while (http_server->is_running()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	http.detach();
	msg = json_value["joke"].asString();
	return true;
}
bool CommandHandler::handle_command_quote(const std::string &args, std::string &msg,
												 const Permission &permission)
{
	HttpClient *http_server = new HttpClient();
	// Key default  A CHANGER
	const std::string url = "http://q.uote.me/api.php?p=json&l=1&s=random";
	Json::Value json_value;
	std::thread http([http_server, url, &json_value] { http_server->get_json(json_value, url); });
	while (http_server->is_running()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	http.detach();
	msg = json_value["data"][0]["text"].asString();
	return true;
}

bool CommandHandler::handle_command_gitlab_issue(const std::string &args, std::string &msg,
		const Permission &permission)
{
	std::cout << "Gitlab handler" << std::endl;
	uint32_t issue_id;
	try {
		issue_id = std::stoi(args);
	} catch (std::invalid_argument &) {
		msg = "Invalid argument.";
		std::cerr << "Invalid argument." << std::endl;
		return false;
	}

	std::cout << "Load issue" << std::endl;

	std::string gitlab_project = m_cfg->get_channel_gitlab_project_name(m_cfg->get_irc_channel_configs().begin()->first);
	std::string gitlab_ns = m_cfg->get_channel_gitlab_project_namespace(m_cfg->get_irc_channel_configs().begin()->first);

	if (gitlab_project == "" || gitlab_ns == "") {
		msg = "Invalid gitlab project";
		return false;
	}

	// Add CACHE (WIP)

	// End cache

	std::cout << "project : " << gitlab_project << " ns : " << gitlab_ns
			<< "uri : " << m_cfg->get_gitlab_uri() << " key : " << m_cfg->get_gitlab_api_key() << std::endl;

	GitlabAPIClient gitlab_client(m_cfg->get_gitlab_uri(),
			m_cfg->get_gitlab_api_key());
	Json::Value result;
	uint32_t project_id = get_gitlab_project_id(gitlab_project, gitlab_ns, gitlab_client);

	if (project_id == 0 || gitlab_client.get_issue(project_id, issue_id, result) != GITLAB_RC_OK) {
		msg = "This issue does not exist";
		return true;
	}

	std::stringstream message;
	message << std::string("Issue #") << issue_id
		   << " (par " << result["author"]["name"].asString()
		   << ", " << result["state"].asString() << "): " << result["title"].asString()
		   << " => " << result["web_url"].asString() << std::endl;
	msg = message.str();
	return true;
}

bool CommandHandler::handle_command_mail(const std::string &args, std::string &msg, const Permission &permission)
{
	std::cout << "Command handle mail" << std::endl;
	std::cout << "args : " << args << std::endl;

	std::string pseudo = "";
	std::string message = "";
	const char *a = args.c_str();

	while (*a != ' ' && *a != '\0') {
		pseudo += *a;
		++a;
	}
	a++;

	while (*a != '\0') {
		message += *a;
		++a;
	}

	if (pseudo == "" || message == "") {
		msg = "Usage : .email <pseudo> <message>";
		return false;
	}

	msg = "Send message to " + pseudo;
	Mail::add_mail()
	return true;
}

uint32_t CommandHandler::get_gitlab_project_id(const std::string &project, const std::string &ns,
											   GitlabAPIClient &gitlab_client)
{
	uint32_t project_id = 0;

	std::string proj_res = "";
	Json::Value p_result;
	GitlabRetCod rc = gitlab_client.get_project_ns(project, ns, p_result);
	if (rc != GITLAB_RC_OK) {
		return 0;
	}

	project_id = p_result["id"].asUInt();

	return project_id;
}
