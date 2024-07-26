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
#include <stdlib.h>
#include <ctype.h>

#include "mac_helper.h"

#define MULTICAST_BYTE_0 0x01
#define MULTICAST_BYTE_1 0x00
#define MULTICAST_BYTE_2 0x5E

static unsigned char hex_digit(char ch)
{
	if ((ch >= '0') && (ch <= '9')) return ch - '0';
	else if ((ch >= 'a') && (ch <= 'f')) return ch - 'a' + 10;
	else if ((ch >= 'A') && (ch <= 'F')) return ch - 'A' + 10;
	else return 0;
}

bool mac_str_to_mac(const char *mac_str, uint8_t *mac)
{
	int i = 0;
	int s = 0;
	char hex_str[13];

	while (*mac_str) {
		if (isxdigit(*mac_str)) {
			hex_str[i] = *mac_str;
			i++;
		} else if (*mac_str == ':') {
			if (i == 0 || i / 2 - 1 != s)
				break;
			s++;
		} else {
			return false;
		}
		mac_str++;
	}
	if ((i != 12) || (s != 0 && s != 5)) return false;

	for (int byte = 0; byte < 6; byte++)
		mac[byte] = (hex_digit(hex_str[2 * byte]) << 4) + hex_digit(hex_str[2 * byte + 1]);
	return true;
}

bool is_all_zeros_mac(uint8_t *mac)
{
	bool is_all_zeros = true;

	for (int i = 0; i < 6; i++)
		if (mac[i] != 0x00) is_all_zeros = false;
	return is_all_zeros;
}

bool is_all_ff_mac(uint8_t *mac)
{
	bool is_all_ff = true;

	for (int i = 0; i < 6; i++)
		if (mac[i] != 0xFF) is_all_ff = false;
	return is_all_ff;
}

bool is_multicast_mac(uint8_t *mac)
{
	if ((mac[0] == MULTICAST_BYTE_0) &&
	    (mac[1] == MULTICAST_BYTE_1) &&
	    (mac[2] == MULTICAST_BYTE_2))
		return true;
	return false;
}
