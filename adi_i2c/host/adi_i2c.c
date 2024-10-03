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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "adi_i2c.h"

#define TA_ADI_I2C_UUID \
	{ \
		0x7e078f09, \
		0xe8cb, 0x47ac, \
		{ \
			0xbc, 0x44, \
			0xfc, 0x6f, \
			0x09, 0x17, \
			0x43, 0x57, \
		} \
	}

/* Op parameter offsets */
#define OP_PARAM_I2C 0
#define OP_PARAM_BUFFER 1

typedef struct i2c_params {
	uint64_t bus;
	uint64_t slave;
	uint64_t address;
	uint64_t length;
	uint64_t set_bytes;
	uint64_t get_bytes;
	uint64_t speed;
} i2c_params_t;

/**
 * adi_i2c_get - Open a TEE session to read/write memory addresses
 */
TEEC_Result adi_i2c_get(uint64_t bus, uint64_t slave, uint64_t speed, uint64_t address, uint64_t length, uint64_t bytes, uint8_t *buf)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_ADI_I2C_UUID;
	uint32_t err_origin;
	TEEC_SharedMemory output_buf;
	TEEC_SharedMemory param_buf;
	uint8_t *data = NULL;
	i2c_params_t i2c_params;

	i2c_params.bus = bus;
	i2c_params.slave = slave;
	i2c_params.speed = speed;
	i2c_params.address = address;
	i2c_params.length = length;
	i2c_params.get_bytes = bytes;
	i2c_params.set_bytes = 0;

	/* Initialize data structure for shared buffer */
	memset((void *)&output_buf, 0, sizeof(output_buf));
	memset((void *)&param_buf, 0, sizeof(param_buf));

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

	/* Buffer to share with OP-TEE TA */
	data = malloc(bytes);
	if (data == NULL) {
		TEEC_FinalizeContext(&ctx);
		TEEC_CloseSession(&sess);
		return TEEC_ERROR_GENERIC;
	}

	/* Register shared memory */
	output_buf.buffer = data;
	output_buf.size = bytes;
	output_buf.flags = TEEC_MEM_OUTPUT;

	param_buf.buffer = &i2c_params;
	param_buf.size = sizeof(i2c_params_t);
	param_buf.flags = TEEC_MEM_INPUT;

	res = TEEC_RegisterSharedMemory(&ctx, &output_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		free(data);
		return res;
	}

	res = TEEC_RegisterSharedMemory(&ctx, &param_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE);
	op.params[OP_PARAM_I2C].memref.parent = &param_buf;
	op.params[OP_PARAM_I2C].memref.size = sizeof(i2c_params_t);
	op.params[OP_PARAM_BUFFER].memref.parent = &output_buf;
	op.params[OP_PARAM_BUFFER].memref.size = bytes;

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, TA_ADI_I2C_GET, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		printf("tee_i2c_get failed with code 0x%x origin 0x%x\n", res, err_origin);
	else
		memcpy(buf, data, bytes);

	/* Close the session and destroy the context */
	free(data);
	TEEC_ReleaseSharedMemory(&output_buf);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}

TEEC_Result adi_i2c_set(uint64_t bus, uint64_t slave, uint64_t speed, uint64_t address, uint64_t length, uint64_t bytes, uint8_t *buf)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_ADI_I2C_UUID;
	uint32_t err_origin;
	TEEC_SharedMemory input_buf;
	TEEC_SharedMemory param_buf;
	i2c_params_t i2c_params;

	i2c_params.bus = bus;
	i2c_params.slave = slave;
	i2c_params.speed = speed;
	i2c_params.address = address;
	i2c_params.length = length;
	i2c_params.get_bytes = 0;
	i2c_params.set_bytes = bytes;

	/* Initialize data structure for shared buffer */
	memset((void *)&input_buf, 0, sizeof(input_buf));
	memset((void *)&param_buf, 0, sizeof(param_buf));

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

	/* Register shared memory */
	input_buf.buffer = buf;
	input_buf.size = bytes;
	input_buf.flags = TEEC_MEM_INPUT;

	param_buf.buffer = &i2c_params;
	param_buf.size = sizeof(i2c_params_t);
	param_buf.flags = TEEC_MEM_INPUT;

	res = TEEC_RegisterSharedMemory(&ctx, &input_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	res = TEEC_RegisterSharedMemory(&ctx, &param_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE);
	op.params[OP_PARAM_I2C].memref.parent = &param_buf;
	op.params[OP_PARAM_I2C].memref.size = sizeof(i2c_params_t);
	op.params[OP_PARAM_BUFFER].memref.parent = &input_buf;
	op.params[OP_PARAM_BUFFER].memref.size = bytes;

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, TA_ADI_I2C_SET, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		printf("tee_i2c_set failed with code 0x%x origin 0x%x\n", res, err_origin);

	/* Close the session and destroy the context */
	TEEC_ReleaseSharedMemory(&input_buf);
	TEEC_ReleaseSharedMemory(&param_buf);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}

TEEC_Result adi_i2c_set_get(uint64_t bus, uint64_t slave, uint64_t speed, uint64_t address, uint64_t length, uint64_t bytes, uint64_t read_bytes, uint8_t *buf)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_ADI_I2C_UUID;
	uint32_t err_origin;
	TEEC_SharedMemory inout_buf;
	TEEC_SharedMemory param_buf;
	i2c_params_t i2c_params;

	i2c_params.bus = bus;
	i2c_params.slave = slave;
	i2c_params.speed = speed;
	i2c_params.address = address;
	i2c_params.length = length;
	i2c_params.get_bytes = bytes;
	i2c_params.set_bytes = read_bytes;

	/* Initialize data structure for shared buffers */
	memset((void *)&inout_buf, 0, sizeof(inout_buf));
	memset((void *)&param_buf, 0, sizeof(param_buf));

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

	/* Register shared memory */
	inout_buf.buffer = buf;
	inout_buf.size = bytes;
	inout_buf.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	param_buf.buffer = &i2c_params;
	param_buf.size = sizeof(i2c_params_t);
	param_buf.flags = TEEC_MEM_INPUT;

	res = TEEC_RegisterSharedMemory(&ctx, &inout_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	res = TEEC_RegisterSharedMemory(&ctx, &param_buf);
	if (res != TEEC_SUCCESS) {
		printf("TEEC_RegisterSharedMemory failed with code 0x%x\n", res);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		return res;
	}

	/* Prepare the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE);
	op.params[OP_PARAM_I2C].memref.parent = &param_buf;
	op.params[OP_PARAM_I2C].memref.size = sizeof(i2c_params_t);
	op.params[OP_PARAM_BUFFER].memref.parent = &inout_buf;
	op.params[OP_PARAM_BUFFER].memref.size = bytes;

	/* Invoke the function */
	res = TEEC_InvokeCommand(&sess, TA_ADI_I2C_SET_GET, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		printf("tee_i2c_set_get failed with code 0x%x origin 0x%x\n", res, err_origin);

	/* Close the session and destroy the context */
	TEEC_ReleaseSharedMemory(&inout_buf);
	TEEC_ReleaseSharedMemory(&param_buf);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return res;
}
