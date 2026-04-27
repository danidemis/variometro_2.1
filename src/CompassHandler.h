#ifndef COMPASS_HANDLER_H
#define COMPASS_HANDLER_H

#include <Arduino.h>
#include <Wire.h>

class CompassHandler {
public:
    void begin();
    void update();
    int getHeading(); // Restituisce i gradi da 0 a 359

private:
    int _heading = 0;
    
    // Variabili per il filtro passa-basso
    float _smoothX = 0;
    float _smoothY = 0;
};

#endif