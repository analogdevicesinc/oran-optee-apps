/*
 * Copyright (c) 2022, Analog Devices Inc.
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

#ifndef TA_EXAMPLE_REG_H
#define TA_EXAMPLE_REG_H


/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_EXAMPLE_REG_UUID \
	{ \
		0xf2fe607c, \
		0x26a1, 0x48ee, \
		{ \
			0x94, 0x55, \
			0x5f, 0xf9, \
			0x49, 0xe4, \
			0xb6, 0x17, \
		} \
	}

/* The function IDs implemented in this TA */
enum ta_example_reg_cmds {
	TA_EXAMPLE_REG_CMD_DUMMY,
	/* New commands go above this comment.
	 * Keep 'COUNT' as the last entry. */
	TA_EXAMPLE_REG_CMDS_COUNT
};

#endif /* TA_EXAMPLE_REG_H */
