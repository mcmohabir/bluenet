/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 13 Dec., 2019
 * License: LGPLv3, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define ARDUINO_HANDLER __attribute__ (( section(".arduino_handlers"))) 

void ARDUINO_HANDLER arduinoCommand();

#ifdef __cplusplus
}
#endif
