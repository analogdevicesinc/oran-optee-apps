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

#include "otp_temp.h"

/* Command line arguments */
#define ARG_TEMP_GROUP 1

/* Command help */
#define HELP "\n\
Usage: \n\
  ./optee_app_otp_temp_read {temp_sensor_group_id} \n\
  \n\
  temp_sensor_group_id (u32 hex): \n\
    0 -- TEMP_SENSOR_CLK_ETH_PLL	  \n\
    1 -- TEMP_SENSOR_RF0_1_PLL   \n\
    2 -- TEMP_SENSOR_TX0_1  \n\
    3 -- TEMP_SENSOR_TX2_3  \n\
\n"

/* Functions definition */
bool parse_value32(char *data, uint32_t *value);

/* MAIN */
int main(int argc, char *argv[])
{
	bool ret;
	uint32_t value;
	uint32_t temp_group_id_input;

	/* Check if arguments provided */
	if (argc != 2) {
		printf("Error: Too many arguments provided.\n");
		printf(HELP);
		return 1;
	}

	/* Parse interface */
	ret = parse_value32(argv[ARG_TEMP_GROUP], &temp_group_id_input);
	if (!ret || temp_group_id_input >= TEMP_SENSOR_OTP_SLOT_NUM) {
		printf("Error: Invalid temp_group_id '%s'.\n", argv[ARG_TEMP_GROUP]);
		printf(HELP);
		return 1;
	}

	/* Read TEMP slope or offset from OTP */
	if (adi_read_otp_temp((adrv906x_temp_group_id_t)temp_group_id_input, &value) != TEEC_SUCCESS) {
		printf("Error: failed reading otp temp data.\n");
		return 1;
	}

	printf("OTP value of temp sensor group id %u: 0x%04x\n", temp_group_id_input, value);

	return 0;
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
