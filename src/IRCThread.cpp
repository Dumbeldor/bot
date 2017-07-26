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
#include <chrono>
#include <thread>
#include "IRCThread.h"
#include "CommandHandler.h"
#include "Config.h"
#include "Mail.h"

IRCThread::IRCThread(): Thread(), IRCClient()
{
	load_state();
}

IRCThread::~IRCThread()
{
	save_state();
}


void IRCThread::load_state()
{
}

void IRCThread::save_state() const
{
}


void* IRCThread::run()
{
	Thread::set_thread_name("IRC");

	ThreadStarted();

	while (!stopRequested()) {
		if (!create_session()) {
			log_fatal(irc_log, "Unable to create IRC session, aborting.");
			return nullptr;
		}

		irc_option_set(m_irc_session, LIBIRC_OPTION_STRIPNICKS);

		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_name = Config::get_instance()->get_irc_name();
		}

		std::string server_password = "";
		if (!Config::get_instance()->get_irc_password().empty()) {
			server_password = Config::get_instance()->get_irc_name() + ":" +
							  Config::get_instance()->get_irc_password();
		}

		log_info(irc_log, "Connecting to " << Config::get_instance()->get_irc_server()
										   << ":" << Config::get_instance()->get_irc_port());

		if (irc_connect(m_irc_session, Config::get_instance()->get_irc_server().c_str(),
						Config::get_instance()->get_irc_port(), NULL,
						Config::get_instance()->get_irc_name().c_str(),
						Config::get_instance()->get_irc_name().c_str(), NULL)) {
			log_fatal(irc_log, "Unable to connect to IRC server "
					<< Config::get_instance()->get_irc_server()
					<< ", aborting.");
			return nullptr;
		}

		if (irc_run(m_irc_session)) {
			log_warn(irc_log, "Connection IRC to "
					<< Config::get_instance()->get_irc_server()
					<< " lost, retrying in 30sec. Error was: "
					<< irc_strerror(irc_errno(m_irc_session)));

			std::this_thread::sleep_for(std::chrono::seconds(30));
		}

		if (m_irc_session) {
			irc_destroy_session(m_irc_session);
			m_irc_session = nullptr;
		}
	}

	return nullptr;
}



void IRCThread::on_event_connect(const std::string &origin,
								 const std::vector<std::string> &params)
{
	if (!is_connected()) {
		log_error(irc_log, "Not connected to IRC " << __FUNCTION__);
		return;
	}

	if (params.size() == 0) {
		log_fatal(irc_log, __FUNCTION__ << ": invalid params size");
		return;
	}

	set_name(std::string(params[0]));

	log_info(irc_log, "Connected to IRC (name: " << params[0] << ")");

	Router::get_instance()->on_irc_connection(std::string(params[0]));

	// Join previously registered channels, we have been disconnected
	for (const auto &ch: m_channels) {
		irc_cmd_join(m_irc_session, ch.first.c_str(), NULL);
	}
}

/**
 * \param origin the person, who joins the channel. By comparing it with
 *               your own nickname, you can check whether your JOIN
 *               command succeed.
 * \param params[0] mandatory, contains the channel name.
 */
void IRCThread::on_event_join(const std::string &origin,
							  const std::vector<std::string> &params)
{
	if (!is_connected()) {
		log_error(irc_log, "Not connected to IRC " << __FUNCTION__);
		return;
	}

	if (params.size() == 0) {
		log_fatal(irc_log, __FUNCTION__ << ": invalid params size");
		return;
	}

	if (origin == m_name) {
		// We joined
		log_notice(irc_log, "Channel " << params[0] << " joined.");
		register_channel(params[0]);
		save_state();
		// Ask server for topic
		log_notice(irc_log, "Ask for topic returned: " <<
													   irc_cmd_topic(m_irc_session, params[0].c_str(), NULL));
	}
	else {
		// Somebody joined
	}

	Router::get_instance()->on_irc_channel_join(params[0], origin);
}

/**
 * \param origin the person, who leaves the channel. By comparing it with
 *               your own nickname, you can check whether your PART
 *               command succeed.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] optional, contains the reason message (user-defined).
 */
void IRCThread::on_event_part(const std::string &origin,
							  const std::vector<std::string> &params)
{
	if (!is_connected()) {
		log_error(irc_log, "Not connected to IRC " << __FUNCTION__);
		return;
	}

	if (params.size() == 0) {
		log_fatal(irc_log, __FUNCTION__ << ": invalid params size");
		return;
	}

	if (origin == m_name) {
		// We joined
		log_notice(irc_log, "Channel " << params[0] << " left.");
		on_channel_leave(std::string(params[0]));
		save_state();
	}
	else {
		// Somebody joined
	}

	std::string reason = "";
	if (params.size() == 2) {
		reason = std::string(params[1]);
	}

	Router::get_instance()->on_irc_channel_part(params[0], origin, reason);
}

/**
 * \param origin the person, who generates the message.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] optional, contains the message text
 */
void IRCThread::on_event_message(const std::string &origin,
								 const std::vector<std::string> &params)
{
	if (!is_connected()) {
		std::cerr << "Error: not connected to IRC on " << __FUNCTION__ << std::endl;
		return;
	}

	if (params.size() == 0) {
		log_fatal(irc_log, __FUNCTION__ << ": invalid params size");
		return;
	}

	// Ignore own messages and empty messages
	if (origin == m_name || params.size() == 1) {
		return;
	}

	// If channel is private channel empty it
	std::string channel = params[0];
	if (channel.compare(m_name) == 0) {
		channel = "";
	}

	Router::get_instance()->on_irc_message(channel, origin, params[1]);
}

void IRCThread::on_event_notice(const std::string &origin,
								const std::vector<std::string> &params)
{
	if (!is_connected()) {
		std::cerr << "Error: not connected to IRC on " << __FUNCTION__ << std::endl;
		return;
	}

	if (params.size() == 0) {
		log_fatal(irc_log, __FUNCTION__ << ": invalid params size");
		return;
	}

	if (origin == "NickServ") {
		if (strstr(params[1].c_str(), "This nickname is registered") == params[1].c_str()) {
			std::string ident_str = "IDENTIFY " + Config::get_instance()->get_irc_password();
			irc_cmd_msg(m_irc_session, origin.c_str(), ident_str.c_str());
		}
		else if (strstr(params[1].c_str(), "You are now identified for") == params[1].c_str()) {
			log_notice(irc_log, "IRC authentication succeed.");
		}
		else if (strstr(params[1].c_str(), "Invalid password for") == params[1].c_str()) {
			log_error(irc_log, "Invalid IRC password!");
		}
	}

	Router::get_instance()->on_irc_notice(params[0], origin, params[1]);
}

/*!
 * The "kick" event is triggered upon receipt of a KICK message, which
 * means that someone on a channel with the client (or possibly the
 * client itself!) has been forcibly ejected.
 *
 * \param origin the person, who kicked the poor.
 * \param params[0] mandatory, contains the channel name.
 * \param params[0] optional, contains the nick of kicked person.
 * \param params[1] optional, contains the kick text
 */
void IRCThread::on_event_kick(const std::string &origin,
							  const std::vector<std::string> &params)
{
	log_debug(irc_log, __FUNCTION__);
	if (!is_connected()) {
		std::cerr << "Error: not connected to IRC on " << __FUNCTION__ << std::endl;
		return;
	}

	if (params.size() < 2) {
		log_fatal(irc_log, __FUNCTION__ << ": invalid params size");
		return;
	}

	const std::string &channel_name = params[0];

	if (params[1] == m_name) {
		log_warn(irc_log, "I was kicked from " << channel_name << ", trying to re-join.");

		// Remove channel from registered list
		on_channel_leave(channel_name);
		save_state();

		if (irc_cmd_join(m_irc_session, params[0].c_str(), NULL)) {
			log_error(irc_log, "Unable to join channel " << channel_name << ", ignoring.");
		}
	}

	Router::get_instance()->on_irc_channel_kick(channel_name, params[1]);
}

void IRCThread::on_event_topic(const std::string &origin,
							   const std::vector<std::string> &params)
{
	log_debug(irc_log, __FUNCTION__);
	if (!is_connected()) {
		std::cerr << "Error: not connected to IRC on " << __FUNCTION__ << std::endl;
		return;
	}

	if (params.size() < 1) {
		log_fatal(irc_log, __FUNCTION__ << ": invalid params size");
		return;
	}

	std::string new_topic = "";
	if (params.size() >= 2) {
		new_topic = params[1];
	}

	Router::get_instance()->on_irc_topic(params[0], origin, new_topic);
}

void IRCThread::on_event_numeric(uint32_t event_id, const std::string &origin,
								 const std::vector<std::string> &params)
{
	switch (event_id) {
		case LIBIRC_RFC_RPL_TOPIC:
		case LIBIRC_RFC_RPL_NOTOPIC: {
			if (params.size() != 3) {
				log_error(irc_log, __FUNCTION__ << ": invalid params size for event_id "
												<< event_id)
				return;
			}

			std::string topic;
			if (params.size() == 3) {
				topic = params[2];
			}

			set_channel_topic(params[1], topic);
			Router::get_instance()->on_irc_topic(params[1], origin, topic);
			break;
		}
		case LIBIRC_RFC_RPL_NAMREPLY: {
			if (params.size() != 4) {
				log_error(irc_log, __FUNCTION__ << ": invalid params size for event_id "
												<< event_id)
				return;
			}

			const std::string &channel_name = params[2];
			if (m_names_queues.find(channel_name) == m_names_queues.end()) {
				m_names_queues[channel_name] = std::queue<std::string>();
			}

			std::vector<std::string> names;
			str_split(params[3], ' ', names);
			for (const auto &n: names) {
				m_names_queues[channel_name].push(n);
			}
			break;
		}
		case LIBIRC_RFC_RPL_ENDOFNAMES: {
			if (params.size() != 3) {
				log_error(irc_log, __FUNCTION__ << ": invalid params size for event_id "
												<< event_id)
				return;
			}

			const std::string &channel_name = params[1];
			auto nq = m_names_queues.find(channel_name);
			std::vector<std::string> channel_members;
			if (nq != m_names_queues.end()) {
				while (!nq->second.empty()) {
					channel_members.push_back(nq->second.front());
					nq->second.pop();
				}

				// Remove callback result from response cache
				m_names_queues.erase(nq);
			}

			Router::get_instance()->on_irc_channel_members(channel_name, channel_members);
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_channels[channel_name]->members = channel_members;
			}
			break;
		}
			// Ignored params
		case 333: // non-RFC RPL_TOPIC_EXTRA
		case LIBIRC_RFC_RPL_MOTD:
		case LIBIRC_RFC_RPL_MOTDSTART:
		case LIBIRC_RFC_RPL_ENDOFMOTD:
			break;
		default:
//			log_warn(irc_log, __FUNCTION__ << ": Received unhandled event_id "
//				<< event_id);
			break;
	}
}

void IRCThread::register_channel(const std::string &channel_name)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	IRCChannel *channel = new IRCChannel();
	channel->name = channel_name;
	m_channels[channel_name] = channel;
}

void IRCThread::on_channel_leave(const std::string &channel_name)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	auto channel_it = m_channels.find(channel_name);
	if (channel_it != m_channels.end()) {
		delete channel_it->second;
		m_channels.erase(channel_it);
	}
}

void IRCThread::set_channel_topic(const std::string &channel_name,
								  const std::string &topic)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	auto channel_it = m_channels.find(channel_name);
	if (channel_it != m_channels.end()) {
		channel_it->second->topic = topic;
	}
	else {
		log_warn(irc_log, __FUNCTION__
				<< ": setting channel topic on unregistered channel '"
				<< channel_name << "', ignoring.");
	}
}

