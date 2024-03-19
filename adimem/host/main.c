/*
 * Copyright (c) 2023, Analog Devices Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include "adimem.h"

/* Command line arguments */
#define ARG_ADDR 1
#define ARG_SIZE 2
#define ARG_DATA 3

/* Command help */
#define HELP "\n\
Usage: %s address [size [data] ] \n\
  - address: decimal or hexadecimal (stared by 0x) \n\
  - size:    8, 16, 32 (default) \n\
  - data:    decimal or hexadecimal (started by 0x) \n\
\n"

/* Functions definition */
bool parse_value32(char *data, uint32_t *value);
bool parse_value64(char *data, uint64_t *value);

/* MAIN */
int main(int argc, char *argv[])
{
	enum ta_adimem_cmds cmd = TA_ADIMEM_CMD_READ;
	size_t cmd_size = 32;
	uint64_t cmd_address;
	uint32_t cmd_rw_value = 0;

	/* Check at least address is provided */
	if (argc < 2) {
		printf(HELP, argv[0]);
		return 1;
	}

	/* Parse address */
	if (!parse_value64(argv[ARG_ADDR], &cmd_address)) {
		printf("Invalid address '%s'.\n", argv[ARG_ADDR]);
		return 1;
	}

	/* Parse data size */
	if (argc > 2) {
		if (!parse_value64(argv[ARG_SIZE], &cmd_size)) {
			printf("Invalid size '%s'.\n", argv[ARG_SIZE]);
			return 1;
		}
	}

	switch (cmd_size) {
	case  8: break;
	case 16: break;
	case 32: break;
	default:
		printf("Invalid size '%s'.\n", argv[ARG_SIZE]);
		return 1;
	}

	/* Parse value to write */
	if (argc > 3) {
		cmd = TA_ADIMEM_CMD_WRITE;
		if (!parse_value32(argv[ARG_DATA], &cmd_rw_value)) {
			printf("Invalid value '%s'.\n", argv[ARG_DATA]);
			return 1;
		}
	}

	/* Execute tee stuff */
	if (adi_readwrite_memory(cmd, cmd_address, cmd_size, &cmd_rw_value) == TEEC_SUCCESS) {
		if (cmd == TA_ADIMEM_CMD_READ)
			printf("0x%x\n", cmd_rw_value);
		return 0;
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
 * parse_value64 - gets uint64_t from string
 */
bool parse_value64(char *data, uint64_t *value)
{
	char *end;

	*value = strtol(data, &end, 0);
	if (*end != '\0') return 0;
	return 1;
}
