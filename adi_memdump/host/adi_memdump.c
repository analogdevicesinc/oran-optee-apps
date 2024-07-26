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

#include "adi_memdump.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define TA_ADI_MEMDUMP_UUID \
	{ \
		0x39f74b29, \
		0x8507, 0x4142, \
		{ \
			0x8b, 0x8e, \
			0x3d, 0x12, \
			0xeb, 0x9d, \
			0x49, 0x7b, \
		} \
	}

/* The function IDs implemented in this TA */
enum ta_adimem_cmds {
	TA_ADI_MEMDUMP_RECORDS_CMD,
	TA_ADI_MEMDUMP_SIZE_CMD,
	TA_ADI_MEMDUMP_CMD
};

/* Op parameter offsets */
/* adi_memdump_get_num_records */
#define OP_PARAM_RECORDS 0

/* adi_memdump - size command */
#define OP_PARAM_RECORD_NUM 0
#define OP_PARAM_RECORD_SIZE 1

/* adi_memdump - size command */
#define OP_PARAM_BUFFER 0
#define OP_PARAM_RECORD_AND_ADDRESS 1
#define OP_PARAM_WIDTH 2
#define OP_PARAM_ENDIANNESS 3

/**
 * adi_memdump_get_num_records - Open a TEE session to get number of records for memdump
 */
TEEC_Result adi_memdump_get_num_records(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_ADI_MEMDUMP_UUID;
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
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, TA_ADI_MEMDUMP_RECORDS_CMD, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		printf("tee_memdump failed with code 0x%x origin 0x%x\n", res, err_origin);
	else
		/* Print number of records */
		printf("0x%08x\n", op.params[OP_PARAM_RECORDS].value.a);

	/* Close the session and destroy the context */
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}

/**
 * adi_memdump - Open a TEE session to dump memory region of specified record
 */
TEEC_Result adi_memdump(uint64_t record)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_ADI_MEMDUMP_UUID;
	uint32_t err_origin;
	TEEC_SharedMemory output_buf;
	FILE *fp;
	uint32_t size = 0;
	uint8_t *data = NULL;

	/* Initialize data structure for shared buffer */
	memset((void *)&output_buf, 0, sizeof(output_buf));

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
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE);
	op.params[OP_PARAM_RECORD_NUM].value.a = record;

	/* Invoke the function to get size of memdump record */
	res = TEEC_InvokeCommand(&sess, TA_ADI_MEMDUMP_SIZE_CMD, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		printf("tee_memdump_size failed with code 0x%x origin 0x%x\n", res, err_origin);
		TEEC_FinalizeContext(&ctx);
		TEEC_CloseSession(&sess);
		return res;
	}

	/* Get returned size of record */
	size = op.params[OP_PARAM_RECORD_SIZE].value.a;

	/* Buffer to share with OP-TEE TA */
	data = malloc(size);

	if (data == NULL) {
		TEEC_FinalizeContext(&ctx);
		TEEC_CloseSession(&sess);
		return TEEC_ERROR_GENERIC;
	}

	/* Register shared memory */
	output_buf.buffer = data;
	output_buf.size = size;
	output_buf.flags = TEEC_MEM_OUTPUT;

	res = TEEC_RegisterSharedMemory(&ctx, &output_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		free(data);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_VALUE_INOUT, TEEC_VALUE_OUTPUT, TEEC_VALUE_OUTPUT);
	op.params[OP_PARAM_BUFFER].memref.parent = &output_buf;
	op.params[OP_PARAM_BUFFER].memref.size = size;
	op.params[OP_PARAM_RECORD_AND_ADDRESS].value.a = record;

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, TA_ADI_MEMDUMP_CMD, &op, &err_origin);
	if (res != TEEC_SUCCESS) {
		printf("tee_memdump failed with code 0x%x origin 0x%x\n", res, err_origin);
	} else {
		printf("0x%08x 0x%04x 0x%04x 0x%01x\n", op.params[OP_PARAM_RECORD_AND_ADDRESS].value.a, op.params[OP_PARAM_BUFFER].memref.size, op.params[2].value.a, op.params[3].value.a);

		/* Dump memory contents to temp binary file */
		fp = fopen("/tmp/memdump.bin", "wb");
		if (fp == NULL) {
			printf("Unable to open file for memdump\n");
			res = TEEC_ERROR_GENERIC;
			goto end;
		}
		res = chmod("/tmp/memdump.bin", 0640);
		if (res != 0) {
			printf("Unable to change file permissions for /tmp/memdump.bin\n");
			res = TEEC_ERROR_GENERIC;
			goto end;
		}
		res = fwrite(data, 1, op.params[OP_PARAM_BUFFER].memref.size, fp);
		if (res != op.params[OP_PARAM_BUFFER].memref.size) {
			printf("Unable to write to file /tmp/memdump.bin\n");
			res = TEEC_ERROR_GENERIC;
			goto end;
		}
		res = fclose(fp);
		if (res != 0) {
			printf("Unable to close file /tmp/memdump.bin\n");
			res = TEEC_ERROR_GENERIC;
			goto end;
		}
	}

end:
	/* Release shared memory, close the session, and destroy the context */
	free(data);
	TEEC_ReleaseSharedMemory(&output_buf);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}
