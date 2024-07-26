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

#include <stdio.h>

#include "mac_helper.h"
#include "otp_macs.h"

/* Command line arguments */
#define ARG_INTERFACE 1
#define ARG_MAC 2

/* Command help */
#define HELP "\n\
Read/Write MAC address of any interface \n\
\n\
Usage: %s interface [mac] \n\
  - interface: 	decimal ID of the interface: \n\
            	1 - eth-1g \n\
            	2 - eth-fh0 \n\
            	3 - eth-fh1 \n\
            	4 - eth-1g-sec \n\
            	5 - eth-fh0-sec \n\
            	6 - eth-fh1-sec \n\
  - mac:    	MAC address to set. Format: aa:bb:cc:dd:ee:ff\n\
\n"

/* Functions definition */
bool parse_value64(char *data, uint64_t *value);

/* MAIN */
int main(int argc, char *argv[])
{
	bool ret;
	uint64_t interface;
	uint8_t mac[6];

	/* Check at least interface is provided */
	if (argc < 2) {
		printf(HELP, argv[0]);
		return 1;
	}

	/* Parse interface */
	ret = parse_value64(argv[ARG_INTERFACE], &interface);
	if (!ret || interface == 0 || interface > NUM_MAC_ADDRESSES) {
		printf("Invalid interface '%s'.\n", argv[ARG_INTERFACE]);
		return 1;
	}

	/* Parse MAC value to write */
	if (argc > 2) {
		if (!mac_str_to_mac(argv[ARG_MAC], mac)) {
			printf("Invalid MAC address '%s'.\n", argv[ARG_MAC]);
			return 1;
		}
		if (is_all_zeros_mac(mac)) {
			printf("All zeros MAC address not allowed.\n");
			return 1;
		}
		if (is_all_ff_mac(mac)) {
			printf("All FF MAC address not allowed.\n");
			return 1;
		}
		if (is_multicast_mac(mac)) {
			printf("Multicast MAC address not allowed.\n");
			return 1;
		}
		/* Write MAC */
		if (adi_write_otp_mac(interface, mac) != TEEC_SUCCESS) {
			printf("Error writing MAC address.\n");
			return 1;
		}
	}

	/* Read MAC */
	if (adi_read_otp_mac(interface, mac) != TEEC_SUCCESS) {
		printf("Error reading MAC address.\n");
		return 1;
	}

	printf("MAC %u: %02X:%02X:%02X:%02X:%02X:%02X\n", interface, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return 0;
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
