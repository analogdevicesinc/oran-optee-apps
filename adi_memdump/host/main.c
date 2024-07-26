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

#include "adi_memdump.h"

/* Command line arguments */
#define ARG_RECORD 1

/* Command help */
#define HELP "\n\
Usage:  [record number] \n\
  - record number: number of record to memdump to /tmp/memdump.bin \n\
  - if record number not provided, will return total number of records \n\
\n"

bool parse_value32(char *data, uint32_t *value);

/* MAIN */
int main(int argc, char *argv[])
{
	uint32_t cmd_record_num = 0;
	int res = 1;

	if (argc == 1) {
		/* If no additional arguments, get number of records */
		if (adi_memdump_get_num_records() != TEEC_SUCCESS)
			return 1;
		else
			return 0;
	} else if (argc == 2) {
		/* If record number provided, get memdump for record */
		if (!parse_value32(argv[ARG_RECORD], &cmd_record_num)) {
			printf("Invalid record number '%s'.\n", argv[ARG_RECORD]);
			return 1;
		}
		if (adi_memdump(cmd_record_num) != TEEC_SUCCESS)
			return 1;
		else
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
