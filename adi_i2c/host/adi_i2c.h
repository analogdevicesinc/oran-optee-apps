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

#ifndef ADI_I2C_H
#define ADI_I2C_H

#include <tee_client_api.h>

/* The function IDs implemented in this TA */
enum ta_adimem_cmds {
	TA_ADI_I2C_GET,
	TA_ADI_I2C_SET,
	TA_ADI_I2C_SET_GET
};

TEEC_Result adi_i2c_get(uint64_t bus, uint64_t slave, uint64_t speed, uint64_t address, uint64_t length, uint64_t bytes, uint8_t *buf);
TEEC_Result adi_i2c_set(uint64_t bus, uint64_t slave, uint64_t speed, uint64_t address, uint64_t length, uint64_t bytes, uint8_t *buf);
TEEC_Result adi_i2c_set_get(uint64_t bus, uint64_t slave, uint64_t speed, uint64_t address, uint64_t length, uint64_t bytes, uint64_t read_bytes, uint8_t *buf);

#endif /* ADI_I2C_H */
