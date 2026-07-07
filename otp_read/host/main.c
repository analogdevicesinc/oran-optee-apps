/*
 * Copyright (c) 2026, Analog Devices Inc.
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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otp_read.h"

#define PRIMARY_TILE   0
#define SECONDARY_TILE 1

#define ECC_DISABLED 0
#define ECC_ENABLED  1

static void usage(void)
{
	fprintf(stderr,
		"Usage: optee_app_otp_read [OPTIONS] <segment> [mask] [shift]\n"
		"\n"
		"Options:\n"
		"  -t, --tile <tile>  Tile to read from: primary|p|0 or secondary|s|1\n"
		"                     (default: primary)\n"
		"  --ecc              Enable ECC (default)\n"
		"  --no-ecc           Disable ECC\n"
		"  -h, --help         Show this help message\n"
		"\n"
		"Arguments:\n"
		"  segment (u32 hex):  OTP segment address/offset to read\n"
		"  mask (u32 hex):     Mask to apply to the read value (default: 0xFFFFFFFF)\n"
		"  shift (u32 hex):	 Number of bits to right-shift the masked value (default: 0)\n"
		"\n"
		"Example:\n"
		"  optee_app_otp_read -t secondary --no-ecc 0x10 0xFC 2\n"
		"\n");
}

static int parse_tile(const char *arg)
{
	if (strcmp(arg, "primary") == 0 || strcmp(arg, "p") == 0 || strcmp(arg, "0") == 0)
		return PRIMARY_TILE;
	if (strcmp(arg, "secondary") == 0 || strcmp(arg, "s") == 0 || strcmp(arg, "1") == 0)
		return SECONDARY_TILE;
	return -1;
}

int main(int argc, char *argv[])
{
	uint32_t tile = PRIMARY_TILE;
	uint32_t ecc = ECC_ENABLED;
	uint32_t addr = 0;
	uint32_t mask = 0xFFFFFFFF;
	uint32_t shift = 0;
	uint32_t value = 0;
	char *end;
	int opt;

	enum {
		OPT_ECC = 256,
		OPT_NO_ECC,
	};

	static const struct option long_options[] = {
		{ "tile",   required_argument, NULL, 't'	},
		{ "ecc",    no_argument,       NULL, OPT_ECC	},
		{ "no-ecc", no_argument,       NULL, OPT_NO_ECC },
		{ "help",   no_argument,       NULL, 'h'	},
		{ NULL,	    0,		       NULL, 0		},
	};

	while ((opt = getopt_long(argc, argv, "t:h", long_options, NULL)) != -1) {
		switch (opt) {
		case 't':
			if (parse_tile(optarg) < 0) {
				fprintf(stderr, "Error: Invalid tile '%s'.\n", optarg);
				usage();
				return 1;
			}
			tile = parse_tile(optarg);
			break;
		case OPT_ECC:
			ecc = ECC_ENABLED;
			break;
		case OPT_NO_ECC:
			ecc = ECC_DISABLED;
			break;
		case 'h':
			usage();
			return 0;
		default:
			usage();
			return 1;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		fprintf(stderr, "Error: Missing required argument: <segment>.\n");
		usage();
		return 1;
	}

	if (argc > 3) {
		fprintf(stderr, "Error: Too many arguments.\n");
		usage();
		return 1;
	}

	addr = strtol(argv[0], &end, 0);
	if (*end != '\0') {
		fprintf(stderr, "Error: Invalid segment value '%s'.\n", argv[0]);
		usage();
		return 1;
	}

	if (argc >= 2) {
		mask = strtol(argv[1], &end, 0);
		if (*end != '\0') {
			fprintf(stderr, "Error: Invalid mask value '%s'.\n", argv[1]);
			usage();
			return 1;
		}
	}

	if (argc >= 3) {
		shift = strtol(argv[2], &end, 0);
		if (*end != '\0') {
			fprintf(stderr, "Error: Invalid shift value '%s'.\n", argv[2]);
			usage();
			return 1;
		}
	}

	if (adi_read_otp(tile, addr, mask, shift, ecc, &value) != TEEC_SUCCESS) {
		fprintf(stderr, "Error: Failed to read OTP segment 0x%X with mask 0x%X.\n", addr, mask);
		return 1;
	}

	printf("0x%X\n", value);
	return 0;
}
