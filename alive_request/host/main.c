/*
 * Copyright (c) 2024, Analog Devices Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <err.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define ALIVE_REPLY_PTA_UUID \
	{ 0xafbc7ee1, 0x8a5c, 0x4d59, \
	  { 0x89, 0xe1, 0xe1, 0x95, 0x40, 0xf7, 0xf9, 0x83 } }


#define TIMEOUT_S       1
#define LOOP_SLEEP_S    1

volatile sig_atomic_t alive_received = 0;

void handle_signal(int signal)
{
	if (signal == SIGUSR1) alive_received = 1;
}

int Request_alive_reply(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = ALIVE_REPLY_PTA_UUID;
	uint32_t err_origin;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* Open a session to the TA */
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));

	/* Execute a function in the TA by invoking it */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	res = TEEC_InvokeCommand(&sess, 0, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

	/* Close the session and destroy the context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}

int main()
{
	/* Open a connection to the syslog */
	openlog("alive_request", LOG_NDELAY, LOG_USER);

	/* Register signal handler */
	signal(SIGUSR1, handle_signal);

	/* Get number of cores */
	int num_cores = sysconf(_SC_NPROCESSORS_CONF);

	/* Init alive cores track variable */
	uint8_t core_is_alive[num_cores];
	memset(core_is_alive, 0, num_cores * sizeof(uint8_t));

	while (1) {
		for (int core = 0; core < num_cores; core++) {
			alive_received = 0;
			pid_t pid = fork();
			if (pid < 0) {
				perror("Fork failed");
				exit(EXIT_FAILURE);
			} else if (pid == 0) { /* Child process */
				/* Set CPU affinity for the child process */
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(core, &cpuset);
				if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
					perror("sched_setaffinity failed");
					exit(EXIT_FAILURE);
				}

				Request_alive_reply();

				/* Signal to parent */
				kill(getppid(), SIGUSR1);
				exit(0);
			} else { /* Parent process */
				/* Set timeout */
				struct timeval timeout;
				timeout.tv_sec = TIMEOUT_S;
				timeout.tv_usec = 0;

				/* Wait for the signal from the child process with timeout */
				int ret = select(0, NULL, NULL, NULL, &timeout);

				if (ret == -1) { /* Signal received or error */
					if (errno == EINTR) {
						if (alive_received) {
							if (!core_is_alive[core]) {
								syslog(LOG_INFO, "Core %i Alive\n", core);
								core_is_alive[core] = 1;
							}
							int status;
							waitpid(pid, &status, 0);
						}
					} else {
						perror("Select failed");
						exit(EXIT_FAILURE);
					}
				} else { /* Timeout */
					if (core_is_alive[core]) {
						syslog(LOG_ERR, "Core %i not responding to alive request!\n", core);
						core_is_alive[core] = 0;
					}
					kill(pid, SIGKILL);
				}
			}
		}

		sleep(LOOP_SLEEP_S);
	}

	/* Close the connection to the syslog */
	closelog();

	return EXIT_SUCCESS;
}
