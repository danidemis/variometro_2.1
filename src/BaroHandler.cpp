#include "BaroHandler.h"
#include "Config.h"
#include <Wire.h>

BaroHandler::BaroHandler() : _ms5611(0x77) {}

bool BaroHandler::begin() {
    Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
    return _ms5611.begin();
}

int BaroHandler::update() {
    return _ms5611.read();
}

float BaroHandler::getPressure() {
    return _ms5611.getPressure();
}