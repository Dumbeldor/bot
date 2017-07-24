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

#include <iostream>
#include "IRCThread.h"
#include "Console.h"
#include "HttpClient.h"
#include "Config.h"
#include <cstring>
#include <thread>
#include <log4cplus/logger.h>

int main (int argc, char **argv)
{
	log4cplus::Logger logger = log4cplus::Logger::getRoot();
	log4cplus::Logger irc_log = logger.getInstance(LOG4CPLUS_TEXT("irc"));


	Config *cfg = new Config();
	if (!cfg->load_configuration()) {
		return 1;
	}

	IRCThread *irc_thread = nullptr;
	std::thread irc;

	if (cfg->is_irc_enabled()) {
		irc_thread = new IRCThread(cfg);
		irc = std::thread([irc_thread, cfg] {
			irc_thread->run(cfg);
		});
	}

	Console *console = new Console(irc_thread);
	std::thread co([console, cfg] { console->run(cfg); });

	while(console->is_running()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	if (irc_thread) {
		irc_thread->stop();
		co.join();
	}

	irc.join();

	delete cfg;

	return 1;
}
