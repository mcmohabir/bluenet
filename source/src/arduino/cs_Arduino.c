#include <arduino/cs_Arduino.h>

#include <drivers/cs_Serial.h>

/** 
 * Arduino command.
 */
static void arduinoCommand(const char value) {
	LOGd("Arduino command %i", value);
}

REGISTER_ARDUINO_HANDLER(arduinoCommand);
