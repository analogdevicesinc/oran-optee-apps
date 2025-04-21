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
  ./optee_app_otp_temp_read <group_id> [group_id] ... \n\
\n\
  temp_sensor_group_id (u32 hex): \n\
    0  -- TEMP_SENSOR_CLK_ETH_PLL   	\n\
    1  -- TEMP_SENSOR_RF0_1_PLL     	\n\
    2  -- TEMP_SENSOR_TX0_1         	\n\
    3  -- TEMP_SENSOR_TX2_3         	\n\
    4  -- TEMP_SENSOR_PLL_SLOPE     	\n\
    5  -- TEMP_SENSOR_TX_SLOPE      	\n\
    6  -- SEC_TEMP_SENSOR_CLK_ETH_PLL   \n\
    7  -- SEC_TEMP_SENSOR_RF0_1_PLL     \n\
    8  -- SEC_TEMP_SENSOR_TX0_1         \n\
    9  -- SEC_TEMP_SENSOR_TX2_3         \n\
   10  -- SEC_TEMP_SENSOR_PLL_SLOPE     \n\
   11  -- SEC_TEMP_SENSOR_TX_SLOPE      \n\
\n"

/* Functions definition */
bool parse_value32(char *data, uint32_t *value);

/* MAIN */
int main(int argc, char *argv[])
{
	int status = 0;
	bool ret = false;
	unsigned int i = 0;
	unsigned int group_count = argc - ARG_TEMP_GROUP;
	uint32_t *values = NULL;
	uint32_t *group_ids = NULL;

	/* Check if arguments provided */
	if (group_count == 0) {
		fprintf(stderr, "Error: At least one group_id must be provided.\n");
		fprintf(stderr, HELP);
		status = 1;
		goto exit;
	}

	values = malloc(sizeof(uint32_t) * group_count);
	group_ids = malloc(sizeof(uint32_t) * group_count);
	if (!values || !group_ids) {
		fprintf(stderr, "Error: Memory allocation failed.\n");
		status = 1;
		goto exit;
	}

	/* Parse interface */
	for (i = 0; i < group_count; i++) {
		const char *arg = argv[ARG_TEMP_GROUP + i];
		ret = parse_value32(arg, &group_ids[i]);
		if (!ret || group_ids[i] >= TEMP_SENSOR_OTP_SLOT_NUM) {
			fprintf(stderr, "Error: Invalid group_id '%s'.\n", arg);
			fprintf(stderr, HELP);
			status = 1;
			goto exit;
		}
	}

	/* Read TEMP slope or offset from OTP */
	for (i = 0; i < group_count; i++) {
		if (adi_read_otp_temp((adrv906x_temp_group_id_t)group_ids[i], &values[i]) != TEEC_SUCCESS) {
			fprintf(stderr, "Error: failed reading otp temp data group_id '%u'.\n", group_ids[i]);
			status = 1;
			goto exit;
		}
	}

	for (i = 0; i < group_count; i++)
		printf("group id %u: 0x%08x\n", group_ids[i], values[i]);

exit:
	if (values)
		free(values);
	if (group_ids)
		free(group_ids);
	return status;
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
