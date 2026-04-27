#ifndef BARO_HANDLER_H
#define BARO_HANDLER_H

#include <MS5611.h>

class BaroHandler {
public:
    BaroHandler();
    bool begin();
    int update();
    float getPressure();
private:
    MS5611 _ms5611;
};

#endif