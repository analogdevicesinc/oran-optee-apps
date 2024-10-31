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
#include <stdlib.h>
#include <string.h>
#include <tee_client_api.h>

#include "adi_i2c.h"

/* Command help */
#define HELP "\n\
Usage: \n\
  get <b> <s> <sp> <a> <l> <n> [-f <path>]\n\
  - Reads (dec)<n> bytes from address (hex)<a> (<l> address length) of slave (hex)<s> in I2C bus <b> with speed <sp> (Hz)\n\
    Values are written to the file specified by <path> if -f <path> is provided\n\
	Supports I2C speed 21 kHz to 400 kHz\n\
  set <b> <s> <sp> <a> <l> <n> [ <b1> ... <bn> | -f <path> ]\n\
  - Writes (dec)<n> bytes (hex)<bX> or from file specified by <path> to address (hex)<a> (<l> address length) of slave (hex)<s>\n\
    in I2C with speed <sp> (Hz)\n\
	Supports I2C speed 21 kHz to 400 kHz\n\
  set-get <b> <s> <sp> <a> <l> <n1> <n2> -f <path> \n\
  - Writes (dec)<n1> bytes (hex)<bX> or from file specified by <path> to address (hex)<a> (<l> address length) of slave (hex)<s>\n\
    in I2C with speed <sp> (Hz) and reads back (dec)<n2> bytes\n\
	Supports I2C speed 21 kHz to 400 kHz\n\
\n"

#define ARG_I2C_COMMAND 1
#define ARG_I2C_BUS     2
#define ARG_I2C_SLAVE   3
#define ARG_I2C_SPEED   4
#define ARG_I2C_ADDRESS 5
#define ARG_I2C_ADDRESS_LENGTH  6
#define ARG_I2C_NUM_BYTES       7
#define ARG_I2C_WRITE_DATA      8
#define ARG_I2C_FILE     8
#define ARG_I2C_FILE_PATH     9

#define ARG_I2C_COMBO_NUM_READ_BYTES    8
#define ARG_I2C_COMBO_WRITE_DATA        9
#define ARG_I2C_COMBO_FILE      9
#define ARG_I2C_COMBO_FILE_PATH 10

#define ADI_I2C_MAX_BYTES       256

/* Functions definition */
bool parse_value64(char *data, uint64_t *value);
bool parse_hex_value8(char *data, uint8_t *value);

int main(int argc, char *argv[])
{
	enum ta_adimem_cmds cmd = 0;
	uint64_t bus, length, bytes, read_bytes, buf_bytes, speed;
	uint8_t slave, address;
	uint8_t *buf;
	uint8_t num;
	uint8_t i;
	FILE *fp;
	int res;
	bool file = false;
	char *path;

	/* Check at least i2c command is provided */
	if (argc < ARG_I2C_COMMAND + 1) {
		printf("No arguments provided\n");
		printf(HELP);
		return 1;
	}

	/* Get I2C command */
	if (strcmp(argv[ARG_I2C_COMMAND], "get") == 0) {
		cmd = TA_ADI_I2C_GET;
		if (argc != 8 && argc != 10) {
			printf("Invalid number of arguments\n");
			printf(HELP);
			return 1;
		}
	} else if (strcmp(argv[ARG_I2C_COMMAND], "set") == 0) {
		cmd = TA_ADI_I2C_SET;
		if (argc < 9) {
			printf("Invalid number of arguments\n");
			printf(HELP);
			return 1;
		}
	} else if (strcmp(argv[ARG_I2C_COMMAND], "set-get") == 0) {
		cmd = TA_ADI_I2C_SET_GET;
		if (argc < 10) {
			printf("Invalid number of arguments\n");
			printf(HELP);
			return 1;
		}
	} else {
		printf("Invalid I2C command %s\n", argv[ARG_I2C_COMMAND]);
		printf(HELP);
		return -EINVAL;
	}

	/* Parse I2C bus */
	if (!parse_value64(argv[ARG_I2C_BUS], &bus)) {
		printf("Invalid bus '%s'.\n", argv[ARG_I2C_BUS]);
		return -EINVAL;
	}

	/* Parse I2C slave */
	if (!parse_hex_value8(argv[ARG_I2C_SLAVE], &slave)) {
		printf("Invalid slave '%s'.\n", argv[ARG_I2C_SLAVE]);
		return -EINVAL;
	}

	/* Parse I2C speed */
	if (!parse_value64(argv[ARG_I2C_SPEED], &speed)) {
		printf("Invalid speed '%s'.\n", argv[ARG_I2C_SPEED]);
		return -EINVAL;
	}

	/* Parse I2C address */
	if (!parse_hex_value8(argv[ARG_I2C_ADDRESS], &address)) {
		printf("Invalid address '%s'.\n", argv[ARG_I2C_ADDRESS]);
		return -EINVAL;
	}

	/* Parse I2C address length */
	if (!parse_value64(argv[ARG_I2C_ADDRESS_LENGTH], &length)) {
		printf("Invalid length '%s'.\n", argv[ARG_I2C_ADDRESS_LENGTH]);
		return -EINVAL;
	}

	/* Parse I2C number of bytes */
	if (!parse_value64(argv[ARG_I2C_NUM_BYTES], &bytes)) {
		printf("Invalid bytes '%s'.\n", argv[ARG_I2C_NUM_BYTES]);
		return -EINVAL;
	}
	if (bytes > ADI_I2C_MAX_BYTES) {
		printf("Number of bytes specified is above maximum allowed bytes: %d\n", ADI_I2C_MAX_BYTES);
		return -EINVAL;
	}

	/* Execute tee stuff */
	if (cmd == TA_ADI_I2C_GET) {
		/* Create buffer to read/write data */
		buf = malloc(bytes);
		if (buf == NULL) {
			printf("Unable to create temporary buffer\n");
			return 1;
		}

		/* Check if file option passed in, if so, use file path for get/set */
		if (argc > ARG_I2C_FILE) {
			if (strcmp(argv[ARG_I2C_FILE], "-f") == 0) {
				file = true;
				if (argc == (ARG_I2C_FILE_PATH + 1)) {
					path = argv[ARG_I2C_FILE_PATH];
				} else {
					printf("Invalid arguments\n");
					printf(HELP);
					free(buf);
					return -EINVAL;
				}
			}
		}
		/* Call I2C get function with input values */
		if (adi_i2c_get(bus, slave, speed, address, length, bytes, buf) == TEEC_SUCCESS) {
			/* If file option specified, write data to file path */
			if (file) {
				fp = fopen(path, "wb");
				if (fp == NULL) {
					printf("Unable to open file %s\n", path);
					free(buf);
					return 1;
				}
				res = chmod(path, 0640);
				if (res != 0) {
					printf("Unable to change file permissions for %s\n", path);
					free(buf);
					return 1;
				}
				res = fwrite(buf, 1, bytes, fp);
				if (res != bytes) {
					printf("Unable to write to file %s\n", path);
					free(buf);
					return 1;
				}
				res = fclose(fp);
				if (res != 0) {
					printf("Unable to close file %s\n", path);
					free(buf);
					return 1;
				}
			} else {
				/* If file option not selected, print data */
				for (i = 0; i < bytes; i++)
					printf("%02x ", *(buf + i));
				printf("\n");
			}
			return 0;
		}
	} else if (cmd == TA_ADI_I2C_SET) {
		/* Create buffer to read/write data */
		buf = malloc(bytes);
		if (buf == NULL) {
			printf("Unable to create temporary buffer\n");
			return 1;
		}

		/* Check if file option passed in, if so, use file path for get/set */
		if (argc > ARG_I2C_FILE) {
			if (strcmp(argv[ARG_I2C_FILE], "-f") == 0) {
				file = true;
				if (argc == (ARG_I2C_FILE_PATH + 1)) {
					path = argv[ARG_I2C_FILE_PATH];
				} else {
					printf("Invalid arguments\n");
					printf(HELP);
					free(buf);
					return -EINVAL;
				}
			}
		}

		/* If file option specified, read data from file path */
		if (file) {
			fp = fopen(path, "rb");
			if (fp == NULL) {
				printf("Unable to open file for i2c\n");
				free(buf);
				return 1;
			}
			res = fread(buf, 1, bytes, fp);
			if (res != bytes) {
				printf("Unable to read from file %s\n", path);
				free(buf);
				return 1;
			}
			res = fclose(fp);
			if (res != 0) {
				printf("Unable to close file %s\n", path);
				free(buf);
				return 1;
			}
		} else {
			/* If file option not specified, read data from input */
			if (argc != (ARG_I2C_WRITE_DATA + bytes)) {
				printf("Invalid number of arguments\n");
				printf(HELP);
				free(buf);
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
		}

		/* Call I2C set function with input values */
		if (adi_i2c_set(bus, slave, speed, address, length, bytes, buf) == TEEC_SUCCESS) {
			free(buf);
			return 0;
		}
	} else if (cmd == TA_ADI_I2C_SET_GET) {
		/* Parse read bytes */
		if (!parse_value64(argv[ARG_I2C_COMBO_NUM_READ_BYTES], &read_bytes)) {
			printf("Invalid bytes '%s'.\n", argv[ARG_I2C_COMBO_NUM_READ_BYTES]);
			return -EINVAL;
		}
		if (read_bytes > ADI_I2C_MAX_BYTES) {
			printf("Number of bytes specified is above maximum allowed bytes: %d\n", ADI_I2C_MAX_BYTES);
			return -EINVAL;
		}

		/* Set bytes for buffer size */
		buf_bytes = (bytes > read_bytes) ? bytes : read_bytes;

		/* Create buffer to get/set data */
		buf = malloc(buf_bytes);
		if (buf == NULL) {
			printf("Unable to create temporary buffer\n");
			return 1;
		}

		/* Check if file option passed in, if so, use file path for get/set */
		if (argc > ARG_I2C_COMBO_FILE) {
			if (strcmp(argv[ARG_I2C_COMBO_FILE], "-f") == 0) {
				file = true;
				if (argc == (ARG_I2C_COMBO_FILE_PATH + 1)) {
					path = argv[ARG_I2C_COMBO_FILE_PATH];
				} else {
					printf("Invalid arguments\n");
					printf(HELP);
					free(buf);
					return -EINVAL;
				}
			}
		}

		/* If file option specified, read data from file path */
		if (file) {
			fp = fopen(path, "rb");
			if (fp == NULL) {
				printf("Unable to open file for i2c\n");
				free(buf);
				return 1;
			}
			res = fread(buf, 1, read_bytes, fp);
			if (res != read_bytes) {
				printf("Unable to read from file %s\n", path);
				free(buf);
				return 1;
			}
			res = fclose(fp);
			if (res != 0) {
				printf("Unable to close file %s\n", path);
				free(buf);
				return 1;
			}
		} else {
			/* If file option not specified, read data from input */
			if (argc != (ARG_I2C_COMBO_WRITE_DATA + read_bytes)) {
				printf("Invalid number of arguments\n");
				printf(HELP);
				free(buf);
				return 1;
			}
			/* Copy input data for I2C write */
			for (int i = 0; i < read_bytes; i++) {
				if (!parse_hex_value8(argv[i + ARG_I2C_COMBO_WRITE_DATA], &num)) {
					printf("Invalid input value\n");
					free(buf);
					return -EINVAL;
				}
				buf[i] = num;
			}
		}

		/* Call I2C set-get function with input values */
		if (adi_i2c_set_get(bus, slave, speed, address, length, bytes, read_bytes, buf) == TEEC_SUCCESS) {
			/* If file option specified, write data to file path */
			if (file) {
				fp = fopen(path, "wb");
				if (fp == NULL) {
					printf("Unable to open file %s\n", path);
					free(buf);
					return 1;
				}
				res = chmod(path, 0640);
				if (res != 0) {
					printf("Unable to change file permissions for %s\n", path);
					free(buf);
					return 1;
				}
				res = fwrite(buf, 1, bytes, fp);
				if (res != bytes) {
					printf("Unable to write to file %s\n", path);
					free(buf);
					return 1;
				}
				res = fclose(fp);
				if (res != 0) {
					printf("Unable to close file %s\n", path);
					free(buf);
					return 1;
				}
			} else {
				/* If file option not selected, print data */
				for (i = 0; i < read_bytes; i++)
					printf("%02x ", *(buf + i));
				printf("\n");
			}

			free(buf);
			return 0;
		}
	} else {
		printf("Invalid command\n");
		printf(HELP);
		free(buf);
		return -EINVAL;
	}

	free(buf);
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
