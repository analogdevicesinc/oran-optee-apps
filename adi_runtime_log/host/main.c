/*
 * Copyright (c) 2025, Analog Devices Inc.
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

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_SMC_UUID \
	{ \
		0x6dc55088, \
		0x4255, 0x41cc, \
		{ \
			0x9b, 0x49, \
			0x04, 0x53, \
			0x4e, 0x6a, \
			0xc3, 0xa6, \
		} \
	}

#define OP_PARAM_OPTEE_BUFFER 0
#define OP_PARAM_BL31_BUFFER 1

#define GROUP_SEPARATOR '\x1D'  /* ASCII Group Separator */

/* The function IDs implemented in this TA */
enum ta_smc_cmds {
	BL31_RUNTIME_LOG_GET_SIZE,
	OPTEE_RUNTIME_LOG_GET_SIZE,
	RUNTIME_LOG_CMD_GET
};

static void print_buffer(char *buffer, uint32_t size)
{
	int pos;

	for (pos = 0; pos < size; pos++) {
		if (buffer[pos] == GROUP_SEPARATOR)
			printf("\n");
		else
			printf("%c", buffer[pos]);
	}
}

int main(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_SMC_UUID;
	uint32_t err_origin;

	TEEC_SharedMemory optee_buf;
	uint8_t *optee_data = NULL;
	TEEC_SharedMemory bl31_buf;
	uint8_t *bl31_data = NULL;
	int bl31_size = 0;
	int optee_size = 0;

	/* Initialize data structure for shared buffers */
	memset((void *)&optee_buf, 0, sizeof(optee_buf));
	memset((void *)&bl31_data, 0, sizeof(bl31_data));

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/*
	 * Open a session to the TA.
	 */
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	/* Invoke the function to get the size of the BL31 runtime log */
	res = TEEC_InvokeCommand(&sess, BL31_RUNTIME_LOG_GET_SIZE, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		TEEC_FinalizeContext(&ctx);
		TEEC_CloseSession(&sess);
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	}
	bl31_size = op.params[0].value.a;

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	/* Invoke the function to get the size of the OP-TEE runtime log */
	res = TEEC_InvokeCommand(&sess, OPTEE_RUNTIME_LOG_GET_SIZE, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		TEEC_FinalizeContext(&ctx);
		TEEC_CloseSession(&sess);
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	}

	optee_size = op.params[0].value.a;

	/* Create OP-TEE buffer to share with OP-TEE TA */
	optee_data = malloc(optee_size);
	if (optee_data == NULL) {
		TEEC_FinalizeContext(&ctx);
		TEEC_CloseSession(&sess);
		return TEEC_ERROR_GENERIC;
	}

	/* Register shared memory for OP-TEE buffer */
	optee_buf.buffer = optee_data;
	optee_buf.size = optee_size;
	optee_buf.flags = TEEC_MEM_OUTPUT;

	res = TEEC_RegisterSharedMemory(&ctx, &optee_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		free(optee_data);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Create BL31 buffer to share with OP-TEE TA */
	bl31_data = malloc(bl31_size);
	if (bl31_data == NULL) {
		TEEC_FinalizeContext(&ctx);
		TEEC_CloseSession(&sess);
		return TEEC_ERROR_GENERIC;
	}

	/* Register shared memory for BL31 buffer */
	bl31_buf.buffer = bl31_data;
	bl31_buf.size = bl31_size;
	bl31_buf.flags = TEEC_MEM_OUTPUT;

	res = TEEC_RegisterSharedMemory(&ctx, &bl31_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		free(bl31_data);
		free(optee_data);
		TEEC_ReleaseSharedMemory(&optee_buf);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Prepare the TEEC_Operation struct and params */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE);
	op.params[OP_PARAM_OPTEE_BUFFER].memref.parent = &optee_buf;
	op.params[OP_PARAM_OPTEE_BUFFER].memref.size = optee_size;
	op.params[OP_PARAM_BL31_BUFFER].memref.parent = &bl31_buf;
	op.params[OP_PARAM_BL31_BUFFER].memref.size = bl31_size;

	/* Invoke the function to get the BL31 and OP-TEE runtime logs */
	res = TEEC_InvokeCommand(&sess, RUNTIME_LOG_CMD_GET, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	} else {
		/* On success, print out BL31 and OP-TEE logs */
		if (strlen(optee_data) != 0) {
			printf("OP-TEE Buffer\n");
			print_buffer(optee_data, optee_size);
		}
		if (strlen(bl31_data) != 0) {
			printf("BL31 Buffer\n");
			print_buffer(bl31_data, bl31_size);
		}
	}

	/* Free buffers, release shared memory, close the session, and destroy the context */
	free(optee_data);
	free(bl31_data);
	TEEC_ReleaseSharedMemory(&optee_buf);
	TEEC_ReleaseSharedMemory(&bl31_buf);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}
