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

#include <drivers/cs_Serial.h>

typedef void (*arduino_func_t)(const char);

struct arduino_handler {
	arduino_func_t f;
	char *name;
};

#define REGISTER_ARDUINO_HANDLER(func)                         \
	static struct arduino_handler __arduino_handler__ ## func  \
	__attribute((__section__(".arduino_handlers")))            \
	__attribute((__used__)) = {                                \
	    .f = func,                                             \
	    .name = #func,                                         \
	}

extern struct arduino_handler __start_arduino_handlers;
extern struct arduino_handler __stop_arduino_handlers;

/*
__attribute__((used)) static void arduino_init_all() {
	struct arduino_handler *iter = &__start_arduino_handlers;
	for (; iter < &__stop_arduino_handlers; ++iter) {
		LOGw("Registered handler %s", iter->name);
	}
}
*/

#ifdef __cplusplus
}
#endif
