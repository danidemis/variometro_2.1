#ifndef VARIO_FILTER_H
#define VARIO_FILTER_H

#include <Arduino.h>

class VarioFilter {
public:
    VarioFilter(float alphaAlt = 0.1, float alphaVSI = 0.05);
    void update(float pressure, float dt);
    
    float getAltitude() { return _emaAlt; }
    float getVSI()      { return _emaVSI; }

private:
    float _emaAlt = 0;
    float _emaVSI = 0;
    float _alphaAlt;
    float _alphaVSI;
    bool _initialized = false;
};

#endif