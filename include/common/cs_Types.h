/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include <cstdint>

typedef uint8_t* buffer_ptr_t;
typedef uint16_t buffer_size_t;

typedef uint16_t ERR_CODE;

typedef uint8_t boolean_t;

#define CS_BLE_GAP_ADDR_LEN 6

typedef struct __attribute__((__packed__)) {
	uint8_t addr[CS_BLE_GAP_ADDR_LEN]; /**< 48-bit address, LSB format. */
} cs_ble_gap_addr_t;
