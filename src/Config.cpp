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

#include "Config.h"
#include <yaml-cpp/yaml.h>

#define CFG_LOAD(obj, key, type, store) \
	if (obj[key].IsDefined()) { \
    	store = obj[key].as<type>(); \
    }

Config::~Config()
{
	for (auto &channel_config: m_irc_channel_configs) {
		delete channel_config.second;
	}
}

bool Config::load_configuration()
{
	YAML::Node config;
	bool valid_config_found = false;
	static const std::vector<std::string> possible_configs = {
			"bot.yml",
			"/usr/local/etc/bot.yml",
			"/etc/bot.yml"
	};

	for (const auto &fname: possible_configs) {
		try {
			config = YAML::LoadFile(fname);
			valid_config_found = true;
			break;
		}
		catch (YAML::BadFile &e) {}
	}

	if (!valid_config_found) {
		std::cerr << "Unable to load configuration from known locations" << std::endl;
		return false;
	}

	CFG_LOAD(config, "log_config", std::string, m_log_config_file);
	if (m_log_config_file.empty()) {
		std::cerr << "Invalid configuration: log_file path is empty!" << std::endl;
		return false;
	}
	YAML::Node http_config = config["http"].as<YAML::Node>();
	YAML::Node irc_config = config["irc"].as<YAML::Node>();
	YAML::Node openweathermap_config = config["openweathermap"].as<YAML::Node>();
	YAML::Node gitlab_config = config["gitlab"].as<YAML::Node>();
	YAML::Node httpd_config = config["httpd"].as<YAML::Node>();
	YAML::Node twitter_config = config["twitter"].as<YAML::Node>();

	try {
		CFG_LOAD(httpd_config, "port", uint16_t, m_httpd_port);
		CFG_LOAD(httpd_config, "enable", bool, m_httpd_enabled);

		CFG_LOAD(http_config, "max_response_size", uint32_t, m_max_http_response_size);

		CFG_LOAD(irc_config, "enable", bool, m_irc_enabled);
		CFG_LOAD(irc_config, "server", std::string, m_irc_server);
		CFG_LOAD(irc_config, "port", uint32_t, m_irc_port);
		CFG_LOAD(irc_config, "name", std::string, m_irc_name);
		CFG_LOAD(irc_config, "password", std::string, m_irc_password);

		std::cout << "Config irc de base chargé" << std::endl;

		if (irc_config["channels"].IsDefined()) {
			for (const auto &channel: irc_config["channels"]) {
				if (!channel["name"].IsDefined()) {
					std::cerr << "Invalid configuration: channel without name!"
							  << std::endl;
					return false;
				}
				std::string channel_name = channel["name"].as<std::string>();
				std::cout << "Chargement config du channel : " << channel_name << std::endl;

				if (m_irc_channel_configs.find(channel_name) != m_irc_channel_configs.end()) {
					std::cerr << "Invalid configuration: duplicate channel '"
							  << channel_name << "' found in list !" << std::endl;
					return false;
				}
				IRCChannelConfig *channel_config = new IRCChannelConfig();
				m_irc_channel_configs[channel_name] = channel_config;
				CFG_LOAD(channel, "passive", bool, channel_config->is_passive);

				std::cout << "Passive : " << channel_config->is_passive << std::endl;

				if (channel["allowed_commands"].IsDefined()) {
					CFG_LOAD(channel, "allowed_commands", std::vector<std::string>,
					channel_config->allowed_commands);
					if (channel_config->allowed_commands.empty()) {
						std::cout << "Al commands allowed" << std::endl;
						channel_config->all_commands_allowed = true;
					}
				}

				CFG_LOAD(channel, "gitlab_project_name", std::string,
						channel_config->gitlab_project_name);

				CFG_LOAD(channel, "gitlab_project_namespace", std::string,
						channel_config->gitlab_project_namespace);

				CFG_LOAD(channel, "gitlab_writers", std::vector<std::string>,
						channel_config->gitlab_writers);

			}
		}

		CFG_LOAD(openweathermap_config, "api_key", std::string, m_openweathermap_api_key);
		// if not key, look at environment variables
		if (m_openweathermap_api_key.empty()) {
			const char *env_api_key = getenv("OWN_API_KEY");
			if (env_api_key) {
				m_openweathermap_api_key = std::string(env_api_key);
			}
		}

		CFG_LOAD(gitlab_config, "api_key", std::string, m_gitlab_api_key);
		CFG_LOAD(gitlab_config, "uri", std::string, m_gitlab_uri);

		CFG_LOAD(twitter_config, "enable", bool, m_twitter_enable);
		CFG_LOAD(twitter_config, "consumer_key", std::string, m_twitter_consumer_key);
		CFG_LOAD(twitter_config, "consumer_secret", std::string, m_twitter_access_token_secret);
		CFG_LOAD(twitter_config, "access_token", std::string, m_twitter_access_token);
		CFG_LOAD(twitter_config, "access_token_secret", std::string, m_twitter_access_token_secret);
		/*
		*/

	}
	catch (std::exception &e) {
		std::cerr << "Invalid configuration file. Please ensure all fields are valid."
				  << std::endl << "Error was: " << e.what() << std::endl;
		return false;
	}
	return true;
}

const std::vector<std::string> Config::get_irc_channels() const
{
	std::vector<std::string> res = {};
	for (const auto &channel: m_irc_channel_configs) {
		res.push_back(channel.first);
	}
	return res;
}

const std::string Config::get_channel_gitlab_project_name(
		const std::string &channel) const
{
	for (const auto &c: m_irc_channel_configs) {
		if (channel.compare(c.first) == 0) {
			return c.second->gitlab_project_name;
		}
	}

	return "";
}

const std::string Config::get_channel_gitlab_project_namespace(
		const std::string &channel) const
{
	for (const auto &c: m_irc_channel_configs) {
		if (channel.compare(c.first) == 0) {
			return c.second->gitlab_project_namespace;
		}
	}

	return "";
}
