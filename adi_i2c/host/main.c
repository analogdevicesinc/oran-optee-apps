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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <tee_client_api.h>

#include "adi_i2c.h"

/* Command help */
#define HELP "\n\
Usage: \n\
  get <b> <s> <sp> <a> <l> <n> \n\
  - Reads (dec)<n> bytes from address (hex)<a> (<l> address length) of slave (hex)<s> in I2C bus <b> with speed <sp>\n\
  set <b> <s> <sp> <a> <l> <n> <b1> ... <bn> \n\
  - Writes (dec)<n> bytes (hex)<bX> to address (hex)<a> (<l> address length) of slave (hex)<s> in I2C with speed <sp>\n\
\n"

#define ARG_I2C_COMMAND 1
#define ARG_I2C_BUS     2
#define ARG_I2C_SLAVE   3
#define ARG_I2C_SPEED   4
#define ARG_I2C_ADDRESS 5
#define ARG_I2C_ADDRESS_LENGTH  6
#define ARG_I2C_NUM_BYTES       7
#define ARG_I2C_WRITE_DATA      8

#define ADI_I2C_MAX_BYTES       256

/* Functions definition */
bool parse_value64(char *data, uint64_t *value);
bool parse_hex_value8(char *data, uint8_t *value);

int main(int argc, char *argv[])
{
	enum ta_adimem_cmds cmd = 0;
	uint64_t bus, length, bytes, speed;
	uint8_t slave, address;
	uint8_t *buf;
	uint8_t num;

	/* Check at least i2c bus is provided */
	if (argc < 2) {
		printf("No arguments provided\n");
		printf(HELP);
		return 1;
	}

	/* Get I2C command */
	if (strcmp(argv[ARG_I2C_COMMAND], "get") == 0) {
		cmd = TA_ADI_I2C_GET;
		if (argc != 8) {
			printf("Invalid number of arguments\n");
			return 1;
		}
	} else if (strcmp(argv[ARG_I2C_COMMAND], "set") == 0) {
		cmd = TA_ADI_I2C_SET;
		if (argc < 9) {
			printf("Invalid number of arguments\n");
			return 1;
		}
	} else {
		printf("Invalid I2C command %s\n", argv[ARG_I2C_COMMAND]);
		printf(HELP);
		return -EINVAL;
	}

	/* Parse I2C bus */
	if (argc > 2) {
		if (!parse_value64(argv[ARG_I2C_BUS], &bus)) {
			printf("Invalid bus '%s'.\n", argv[ARG_I2C_BUS]);
			return -EINVAL;
		}
	}

	/* Parse I2C slave */
	if (argc > 3) {
		if (!parse_hex_value8(argv[ARG_I2C_SLAVE], &slave)) {
			printf("Invalid slave '%s'.\n", argv[ARG_I2C_SLAVE]);
			return -EINVAL;
		}
	}

	/* Parse I2C speed */
	if (argc > 4) {
		if (!parse_value64(argv[ARG_I2C_SPEED], &speed)) {
			printf("Invalid speed '%s'.\n", argv[ARG_I2C_SPEED]);
			return -EINVAL;
		}
	}

	/* Parse I2C address */
	if (argc > 5) {
		if (!parse_hex_value8(argv[ARG_I2C_ADDRESS], &address)) {
			printf("Invalid address '%s'.\n", argv[ARG_I2C_ADDRESS]);
			return -EINVAL;
		}
	}

	/* Parse I2C address length */
	if (argc > 6) {
		if (!parse_value64(argv[ARG_I2C_ADDRESS_LENGTH], &length)) {
			printf("Invalid length '%s'.\n", argv[ARG_I2C_ADDRESS_LENGTH]);
			return -EINVAL;
		}
	}

	/* Parse I2C number of bytes */
	if (argc > 7) {
		if (!parse_value64(argv[ARG_I2C_NUM_BYTES], &bytes)) {
			printf("Invalid bytes '%s'.\n", argv[ARG_I2C_NUM_BYTES]);
			return -EINVAL;
		}
		if (bytes > ADI_I2C_MAX_BYTES) {
			printf("Number of bytes specified is above maximum allowed bytes: %d\n", ADI_I2C_MAX_BYTES);
			return -EINVAL;
		}
	}

	/* Execute tee stuff */
	if (cmd == TA_ADI_I2C_GET) {
		if (adi_i2c_get(bus, slave, speed, address, length, bytes) == TEEC_SUCCESS)
			return 0;
	} else if (cmd == TA_ADI_I2C_SET) {
		if (argc != (ARG_I2C_WRITE_DATA + bytes)) {
			printf("Invalid number of arguments\n");
			return 1;
		}

		buf = malloc(bytes);
		if (buf == NULL) {
			printf("Unable to copy input data\n");
			return 1;
		}

		/* Copy input data for I2C write */
		for (int i = 0; i < bytes; i++) {
			if (!parse_hex_value8(argv[i + ARG_I2C_WRITE_DATA], &num)) {
				printf("Invalid input value\n");
				free(buf);
				return -EINVAL;
			}
			buf[i] = num;
		}
		if (adi_i2c_set(bus, slave, speed, address, length, bytes, buf) == TEEC_SUCCESS) {
			free(buf);
			return 0;
		}
		free(buf);
	} else {
		printf("Invalid command\n");
		return -EINVAL;
	}

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
