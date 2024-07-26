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

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "te_mailbox.h"

/* Command help */
#define HELP "\n\
Usage: [command] [arguments] \n\
  --prov-host-keys: [key id] [key length] [key] \n\
  --prov-prepare-finalize:    no arguments \n\
  --prov-finalize:    no arguments \n\
\n"

/* Functions definition */
bool parse_value32(char *data, uint32_t *value);
bool parse_hex_value8(char *data, uint8_t *value);

/* MAIN */
int main(int argc, char *argv[])
{
	uint32_t type, size;
	uint8_t num;
	uint8_t *key = NULL;
	int ret;

	/* Check if arguments provided */
	if (argc < 2) {
		printf("Error with arguments\n");
		return 1;
	}

	if (strcmp(argv[1], "--prov-host-keys") == 0) {
		printf("provision host keys\n");

		if (argc < 3) {
			printf("Missing key id\n");
			return 1;
		}

		/* Get key id */
		if (!parse_value32(argv[2], &type)) {
			printf("Invalid key id '%s'.\n", argv[2]);
			return 1;
		}

		if (argc < 4)
			printf("Missing key length\n");

		/* Get key length */
		if (!parse_value32(argv[3], &size)) {
			printf("Invalid key length '%s'.\n", argv[3]);
			return 1;
		}

		key = malloc(size);
		if (key == NULL) {
			printf("Unable to copy input key\n");
			return 1;
		}

		/* Verify full key of correct size is present */
		if (argc != (4 + size)) {
			printf("Missing key of size %d\n", size);
			return 1;
		}

		/* Copy key */
		for (int i = 0; i < size; i++) {
			if (!parse_hex_value8(argv[i + 4], &num)) {
				printf("Invalid key value\n");
				return 1;
			}
			key[i] = num;
		}

		/* Call TA to execute provision host key mailbox API */
		if (te_mailbox_prov_host_key(key, type, size) == TEEC_SUCCESS)
			return 0;
	} else if (strcmp(argv[1], "--prov-prepare-finalize") == 0) {
		if (argc != 2) {
			printf(HELP);
			return 1;
		}

		/* Call TA to execute provision prepare finalize mailbox API */
		if (te_mailbox(PROV_PREP_FINALIZE_CMD) == TEEC_SUCCESS)
			return 0;
	} else if (strcmp(argv[1], "--prov-finalize") == 0) {
		if (argc != 2) {
			printf(HELP);
			return 1;
		}

		/* Call TA to execute provision finalize mailbox API */
		if (te_mailbox(PROV_FINALIZE_CMD) == TEEC_SUCCESS)
			return 0;
	} else {
		printf(HELP);
		return 1;
	}

	return 1;
}

/**
 * parse_value32 - gets uint32_t from string
 */
bool parse_value32(char *data, uint32_t *value)
{
	char *end;

	*value = strtol(data, &end, 0);
	if (*end != '\0') return 0;
	return 1;
}

/**
 * parse_hex_value8 - gets hex value from string
 */
bool parse_hex_value8(char *data, uint8_t *value)
{
	char *end;

	*value = (uint8_t)strtol(data, &end, 16);
	if (*end != '\0') return 0;
	return 1;
}
