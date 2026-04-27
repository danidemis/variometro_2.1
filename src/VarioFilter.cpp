#include "VarioFilter.h"
#include <math.h>

VarioFilter::VarioFilter(float alphaAlt, float alphaVSI) 
    : _alphaAlt(alphaAlt), _alphaVSI(alphaVSI) {}

void VarioFilter::update(float pressure, float dt) {
    // Conversione Pressione -> Altitudine (Formula Barometrica)
    float rawAlt = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.190295));

    if (!_initialized) {
        _emaAlt = rawAlt;
        _initialized = true;
        return;
    }

    // 1. Filtro sull'altitudine
    float lastAlt = _emaAlt;
    _emaAlt = (_alphaAlt * rawAlt) + (1.0f - _alphaAlt) * _emaAlt;

    // 2. Calcolo VSI istantanea (m/s)
    float instantVSI = (_emaAlt - lastAlt) / dt;

    // 3. Filtro sulla VSI per eliminare i "salti" audio
    _emaVSI = (_alphaVSI * instantVSI) + (1.0f - _alphaVSI) * _emaVSI;
}