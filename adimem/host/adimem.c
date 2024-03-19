/**
 * Copyright (c) 2023, Analog Devices Inc.
 * All rights reserved.
 */

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
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INOUT, TEEC_NONE);
	op.params[OP_PARAM_ADDR].value.a = address;
	op.params[OP_PARAM_SIZE].value.a = size;
	op.params[OP_PARAM_DATA].value.a = *rw_value;

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
