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
#include <string.h>

#include "otp_macs.h"

#define TA_OTP_MACS_UUID \
	{ \
		0x61e8b041, \
		0xc3bc, 0x4b70, \
		{ \
			0xa9, 0x9e, \
			0xd2, 0xe5, \
			0xba, 0x2c, \
			0x4e, 0xbf, \
		} \
	}

/* Op parameter offsets */
#define OP_PARAM_INTERFACE      0
#define OP_PARAM_MAC_VALUE      1

/* The function IDs implemented in this TA */
enum ta_otp_macs_cmds {
	TA_OTP_MACS_CMD_READ,
	TA_OTP_MACS_CMD_WRITE,
	TA_OTP_MACS_CMDS_COUNT
};

/**
 * adi_readwrite_otp_mac - Open a TEE session to read/write MAC addresses
 */
TEEC_Result adi_readwrite_otp_mac(enum ta_otp_macs_cmds command, uint8_t interface, uint8_t *mac)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_OTP_MACS_UUID;
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
	op.params[OP_PARAM_INTERFACE].value.a = interface;
	op.params[OP_PARAM_MAC_VALUE].value.a = (mac[0] << 8) | mac[1];
	op.params[OP_PARAM_MAC_VALUE].value.b = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, command, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		printf("adi_readwrite_otp_mac failed with code 0x%x origin 0x%x\n", res, err_origin);
	} else {
		mac[0] = (op.params[OP_PARAM_MAC_VALUE].value.a >> 8) & 0xFF;
		mac[1] = (op.params[OP_PARAM_MAC_VALUE].value.a >> 0) & 0xFF;
		mac[2] = (op.params[OP_PARAM_MAC_VALUE].value.b >> 24) & 0xFF;
		mac[3] = (op.params[OP_PARAM_MAC_VALUE].value.b >> 16) & 0xFF;
		mac[4] = (op.params[OP_PARAM_MAC_VALUE].value.b >> 8) & 0xFF;
		mac[5] = (op.params[OP_PARAM_MAC_VALUE].value.b >> 0) & 0xFF;
	}

	/* Close the session and destroy the context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}

TEEC_Result adi_read_otp_mac(uint8_t interface, uint8_t *mac)
{
	return adi_readwrite_otp_mac(TA_OTP_MACS_CMD_READ, interface, mac);
}

TEEC_Result adi_write_otp_mac(uint8_t interface, uint8_t *mac)
{
	return adi_readwrite_otp_mac(TA_OTP_MACS_CMD_WRITE, interface, mac);
}
