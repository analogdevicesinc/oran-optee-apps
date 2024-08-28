/*
 * Copyright (c) 2023, Analog Devices Inc.
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

#include <unistd.h>
#include "adimem.h"

#define TA_ADIMEM_UUID \
	{ \
		0x23fd8eb3, \
		0xf9e6, 0x434c, \
		{ \
			0x94, 0xf2, \
			0xa9, 0x1a, \
			0x61, 0x38, \
			0xbf, 0x3d, \
		} \
	}

/* Op parameter offsets */
#define OP_PARAM_ADDR 0
#define OP_PARAM_SIZE 1
#define OP_PARAM_DATA 2
#define OP_PARAM_PRIV 3

/**
 * adi_readwrite_memory - Open a TEE session to read/write memory addresses
 */
TEEC_Result adi_readwrite_memory(enum ta_adimem_cmds command, uint64_t address, size_t size, uint32_t *rw_value)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_ADIMEM_UUID;
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
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INOUT, TEEC_VALUE_INPUT);
	op.params[OP_PARAM_ADDR].value.a = address;
	op.params[OP_PARAM_SIZE].value.a = size;
	op.params[OP_PARAM_DATA].value.a = *rw_value;

	/* If application is running as root, flag this as a "privileged" access to the adimem TA.
	 * adimem TA will only respect this flag if all of the following are true:
	 * 1) adimem TA is part of a debug build
	 * 2) Device lifecycle state is pre-deployed
	 *
	 * Obviously an attacker could write a non-root host application that deliberately sets
	 * the privileged flag. The TA checks listed above are intended to prevent an attacker
	 * from exploiting this in a production image, or a debug image that is deployed in the field.
	 * Also, users must be part of the "tee" user group in order to call OP-TEE TAs.
	 */
	op.params[OP_PARAM_PRIV].value.a = (geteuid() == 0) ? 1 : 0;

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, command, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		printf("tee_readwrite_memory failed with code 0x%x origin 0x%x\n", res, err_origin);
	else
		*rw_value = op.params[OP_PARAM_DATA].value.a;

	/* Close the session and destroy the context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}
