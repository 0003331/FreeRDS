/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2012
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 *
 * @file thread.c
 * @brief thread stuff...
 * @author Simone Fedele
 *
 */

#include "sesman.h"

#include <errno.h>
#include <signal.h>
#include <pthread.h>

extern struct config_sesman *g_cfg; /* in sesman.c */

static pthread_t g_thread_sighandler;
//static pthread_t g_thread_updater;

/* a variable to pass the socket of s connection to a thread */
int g_thread_sck;

int thread_sighandler_start(void)
{
	int ret;
	sigset_t sigmask;
	sigset_t oldmask;
	sigset_t waitmask;

	/* mask signals to be able to wait for them... */
	sigfillset(&sigmask);
	pthread_sigmask(SIG_BLOCK, &sigmask, &oldmask);

	/* unblock some signals... */
	sigemptyset(&waitmask);

	/* it is a good idea not to block SIGILL SIGSEGV */
	/* SIGFPE -- see sigaction(2) NOTES              */
	sigaddset(&waitmask, SIGILL);
	sigaddset(&waitmask, SIGSEGV);
	sigaddset(&waitmask, SIGFPE);
	pthread_sigmask(SIG_UNBLOCK, &waitmask, NULL);

	log_message(LOG_LEVEL_INFO, "starting signal handling thread...");

	ret = pthread_create(&g_thread_sighandler, NULL, sig_handler_thread, "");
	pthread_detach(g_thread_sighandler);

	if (ret == 0)
	{
		log_message(LOG_LEVEL_INFO, "signal handler thread started successfully");
		return 0;
	}

	/* if something happened while starting a new thread... */
	switch (ret)
	{
		case EINVAL:
			log_message(LOG_LEVEL_ERROR,
					"invalid attributes for signal handling thread (creation returned  EINVAL)");
			break;
		case EAGAIN:
			log_message(LOG_LEVEL_ERROR,
					"not enough resources to start signal handling thread (creation returned EAGAIN)");
			break;
		case EPERM:
			log_message(LOG_LEVEL_ERROR,
					"invalid permissions for signal handling thread (creation returned EPERM)");
			break;
		default:
			log_message(LOG_LEVEL_ERROR, "unknown error starting signal handling thread");
	}

	return 1;
}

int thread_scp_start(int skt)
{
	int ret;
	pthread_t th;

	/* blocking the use of thread_skt */
	lock_socket_acquire();
	g_thread_sck = skt;

	/* start a thread that processes a connection */
	ret = pthread_create(&th, NULL, scp_process_start, "");
	pthread_detach(th);

	if (ret == 0)
	{
		log_message(LOG_LEVEL_INFO, "scp thread on sck %d started successfully", skt);
		return 0;
	}

	/* if something happened while starting a new thread... */
	switch (ret)
	{
		case EINVAL:
			log_message(LOG_LEVEL_ERROR,
					"invalid attributes for scp thread on sck %d (creation returned  EINVAL)", skt);
			break;
		case EAGAIN:
			log_message(LOG_LEVEL_ERROR,
					"not enough resources to start scp thread on sck %d (creation returned EAGAIN)", skt);
			break;
		case EPERM:
			log_message(LOG_LEVEL_ERROR,
					"invalid permissions for scp thread on sck %d (creation returned EPERM)", skt);
			break;
		default:
			log_message(LOG_LEVEL_ERROR, "unknown error starting scp thread on sck %d");
	}

	return 1;
}
