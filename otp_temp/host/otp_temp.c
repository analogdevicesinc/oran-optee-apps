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

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_OTP_TEMP_UUID \
	{ \
		0xcf0ba31d, \
		0xa0a8, 0x4406, \
		{ \
			0x9e, 0x8c, \
			0xba, 0x11, \
			0xdf, 0x80, \
			0xfb, 0xb1, \
		} \
	}

/* Op parameter offsets */
#define OP_PARAM_TEMP_GROUP_ID   0
#define OP_PARAM_TEMP_VALUE      1

/* The function IDs implemented in this TA */
typedef enum ta_otp_temp_cmds {
	TA_OTP_TEMP_CMD_READ,
	TA_OTP_TEMP_CMD_WRITE,
	TA_OTP_TEMP_CMDS_COUNT
}ta_otp_temp_cmds_t;

/**
 * adi_readwrite_otp_temp - Open a TEE session to read/write MAC addresses
 */
static TEEC_Result adi_readwrite_otp_temp(ta_otp_temp_cmds_t command, adrv906x_temp_group_id_t temp_group_id, uint32_t *value)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_OTP_TEMP_UUID;
	uint32_t err_origin;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_InitializeContext failed with code 0x%x\n", res);
		return res;
	}

	/* Open a session to the TA. */
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_Opensession failed with code 0x%x origin 0x%x\n", res, err_origin);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE);
	op.params[OP_PARAM_TEMP_GROUP_ID].value.a = temp_group_id;
	op.params[OP_PARAM_TEMP_VALUE].value.a = *value;

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, command, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		printf("TEEC-optee-app failed with code 0x%x origin 0x%x\n", res, err_origin);
	else
		*value = (op.params[OP_PARAM_TEMP_VALUE].value.a) & 0x7FFF7FFF;

	/* Close the session and destroy the context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}

TEEC_Result adi_read_otp_temp(adrv906x_temp_group_id_t temp_group_id, uint32_t *value)
{
	return adi_readwrite_otp_temp(TA_OTP_TEMP_CMD_READ, temp_group_id, value);
}

TEEC_Result adi_write_otp_temp(adrv906x_temp_group_id_t temp_group_id, uint32_t *value)
{
	return adi_readwrite_otp_temp(TA_OTP_TEMP_CMD_WRITE, temp_group_id, value);
}
