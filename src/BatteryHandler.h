#ifndef BATTERY_HANDLER_H
#define BATTERY_HANDLER_H

#include <Arduino.h>

class BatteryHandler {
public:
    void begin();
    void update();
    float getVoltage();
    int getPercentage();

private:
    float _voltage = 0.0;
    int _percentage = 0;
    unsigned long _lastRead = 0;
    
    // Da calibrare in base al partitore di tensione sul tuo PCB
    // Esempio: se hai due resistenze da 100k, il moltiplicatore è circa 2.0
    // L'ESP32-S3 ha l'ADC attenuato, un fattore tipico è da testare col multimetro
    const float ADC_MULTIPLIER = 5.12;
};

#endif