/**
 * Copyright (c) 2023, Analog Devices Inc.
 * All rights reserved.
 */

#ifndef ADIMEM_H
#define ADIMEM_H

#include <tee_client_api.h>

/* The function IDs implemented in this TA */
enum ta_adimem_cmds {
	TA_ADIMEM_CMD_READ,
	TA_ADIMEM_CMD_WRITE,
	TA_ADIMEM_CMDS_COUNT
};

TEEC_Result adi_readwrite_memory(enum ta_adimem_cmds command, uint64_t address, size_t size, uint32_t *rw_value);

#endif /* ADIMEM_H */
