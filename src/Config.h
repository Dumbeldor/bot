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
#include <vector>
#include <unordered_map>

struct IRCChannelConfig
{
	bool is_passive = false;
	bool all_commands_allowed = true;
	std::vector<std::string> allowed_commands = {};
	std::string gitlab_project_name = "";
	std::string gitlab_project_namespace = "";
	std::vector<std::string> gitlab_writers = {};
};

typedef std::unordered_map<std::string, IRCChannelConfig*> IRCChannelConfigs;

class Config {
public:
	Config() {};
	~Config();

	bool load_configuration();

	bool is_httpd_enabled() const
	{
		return m_httpd_enabled;
	}

	void set_httpd_enabled(bool httpd_enabled)
	{
		m_httpd_enabled = httpd_enabled;
	}

	uint16_t get_httpd_port() const
	{
		return m_httpd_port;
	}

	void set_httpd_port(uint16_t httpd_port)
	{
		m_httpd_port = httpd_port;
	}

	const std::string &get_irc_name() const
	{
		return m_irc_name;
	}

	void set_irc_name(const std::string &irc_name)
	{
		m_irc_name = irc_name;
	}

	const std::string &get_irc_password() const
	{
		return m_irc_password;
	}

	void set_irc_password(const std::string &irc_password)
	{
		m_irc_password = irc_password;
	}

	const std::string &get_irc_server() const
	{
		return m_irc_server;
	}

	void set_irc_server(const std::string &irc_server)
	{
		m_irc_server = irc_server;
	}

	bool is_irc_enabled() const
	{
		return m_irc_enabled;
	}

	void set_irc_enabled(bool irc_enabled)
	{
		m_irc_enabled = irc_enabled;
	}

	uint16_t get_irc_port() const
	{
		return m_irc_port;
	}

	void set_irc_port(uint16_t irc_port)
	{
		m_irc_port = irc_port;
	}

	const IRCChannelConfigs &get_irc_channel_configs() const
	{
		return m_irc_channel_configs;
	}

	void set_irc_channel_configs(const IRCChannelConfigs &irc_channel_configs)
	{
		m_irc_channel_configs = irc_channel_configs;
	}

	uint32_t get_max_http_response_size() const
	{
		return m_max_http_response_size;
	}

	void set_max_http_response_size(uint32_t max_http_response_size)
	{
		m_max_http_response_size = max_http_response_size;
	}

	const std::string &get_openweathermap_api_key() const
	{
		return m_openweathermap_api_key;
	}

	void set_openweathermap_api_key(const std::string &openweathermap_api_key)
	{
		m_openweathermap_api_key = openweathermap_api_key;
	}

	const std::string &get_gitlab_api_key() const
	{
		return m_gitlab_api_key;
	}

	void set_gitlab_api_key(const std::string &gitlab_api_key)
	{
		m_gitlab_api_key = gitlab_api_key;
	}

	const std::string &get_gitlab_uri() const
	{
		return m_gitlab_uri;
	}

	void set_gitlab_uri(const std::string &gitlab_uri)
	{
		m_gitlab_uri = gitlab_uri;
	}

	const std::string &getTwitter_consumer_key() const
	{
		return m_twitter_consumer_key;
	}

	void setTwitter_consumer_key(const std::string &twitter_consumer_key)
	{
		m_twitter_consumer_key = twitter_consumer_key;
	}

	const std::string &getTwitter_consumer_secret() const
	{
		return m_twitter_consumer_secret;
	}

	void setTwitter_consumer_secret(const std::string &twitter_consumer_secret)
	{
		m_twitter_consumer_secret = twitter_consumer_secret;
	}

	const std::string &getTwitter_access_token() const
	{
		return m_twitter_access_token;
	}

	void setTwitter_access_token(const std::string &twitter_access_token)
	{
		m_twitter_access_token = twitter_access_token;
	}

	const std::string &getTwitter_access_token_secret() const
	{
		return m_twitter_access_token_secret;
	}

	void setTwitter_access_token_secret(const std::string &twitter_access_token_secret)
	{
		m_twitter_access_token_secret = twitter_access_token_secret;
	}

	const std::vector<std::string> get_irc_channels() const;
	const std::string get_channel_gitlab_project_name(const std::string &channel) const;
	const std::string get_channel_gitlab_project_namespace(const std::string &channel) const;

private:
	bool m_httpd_enabled = true;
	uint16_t m_httpd_port = 8080;
	std::string m_irc_name = "mybot_name";
	std::string m_irc_password = "";
	std::string m_irc_server = "chat.freenode.net";
	bool m_irc_enabled = true;
	uint16_t m_irc_port = 6697;
	IRCChannelConfigs m_irc_channel_configs = {};
	uint32_t m_max_http_response_size = 100 * 1024;
	std::string m_openweathermap_api_key = "";
	std::string m_gitlab_api_key = "";
	std::string m_gitlab_uri = "";

	std::string m_log_config_file = "log4cpp.properties";
	/*
	std::string m_redis_host = "localhost";
	uint16_t m_redis_port = 6379;
	 */

	bool m_twitter_enable = false;
	std::string m_twitter_consumer_key = "";
	std::string m_twitter_consumer_secret = "";
	std::string m_twitter_access_token = "";
	std::string m_twitter_access_token_secret = "";
};
