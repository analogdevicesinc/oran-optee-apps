/**
 * Copyright (c) 2025, Analog Devices Inc.
 * All rights reserved.
 */

#ifndef TE_MAILBOX_H
#define TE_MAILBOX_H

#include <stdint.h>
#include <tee_client_api.h>

#define PROV_HOST_KEY_CMD           0
#define PROV_PREP_FINALIZE_CMD      1
#define PROV_FINALIZE_CMD           2
#define BOOT_FLOW_REG_READ          3

TEEC_Result te_mailbox(uint32_t cmd);
TEEC_Result te_mailbox_prov_host_key(uint8_t *key, uint32_t type, uint32_t size);

#endif /* TE_MAILBOX_H */
