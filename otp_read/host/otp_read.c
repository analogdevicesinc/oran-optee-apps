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

#include "otp_read.h"

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_OTP_READ_UUID \
	{ \
		0xbc9df1f1, \
		0xb43f, 0x4e61, \
		{ \
			0x8a, 0xef, \
			0x18, 0xfb, \
			0x28, 0xa4, \
			0xa2, 0xff, \
		} \
	}

/* The function IDs implemented in this TA */
enum ta_otp_read_cmds {
	TA_OTP_READ_CMD_READ,
	TA_OTP_READ_CMDS_COUNT
};

#define OP_PARAM_TILE   0
#define OP_PARAM_OFFSET 1
#define OP_PARAM_MASK   2
#define OP_PARAM_ECC    3

#define OP_OUT_VALUE    0

#define PRIMARY_TILE    0
#define SECONDARY_TILE  1

#define ECC_DISABLED    0
#define ECC_ENABLED     1

/**
 * adi_read_otp - Open a TEE session to read OTP values
 */
TEEC_Result adi_read_otp(uint32_t tile, uint32_t addr, uint32_t mask, uint32_t shift,
			 uint32_t ecc, uint32_t *value)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_OTP_READ_UUID;
	uint32_t err_origin;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_InitializeContext failed with code 0x%x\n", res);
		return res;
	}

	/* Open a session to the TA. */
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS) {
		fprintf(stderr, "TEEC_Opensession failed with code 0x%x origin 0x%x\n", res, err_origin);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INOUT, TEEC_VALUE_INPUT, TEEC_NONE);
	op.params[OP_PARAM_TILE].value.a = tile;
	op.params[OP_PARAM_OFFSET].value.a = addr;
	op.params[OP_PARAM_MASK].value.a = mask;
	op.params[OP_PARAM_ECC].value.a = ecc;

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, TA_OTP_READ_CMD_READ, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		fprintf(stderr, "TEEC-optee-app failed with code 0x%x origin 0x%x\n", res, err_origin);
	else if (value != NULL)
		*value = (op.params[OP_OUT_VALUE].value.a >> shift) & 0xFFFFFFFF;

	/* Close the session and destroy the context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}
