#ifndef GPS_HANDLER_H
#define GPS_HANDLER_H

#include <Arduino.h>
#include <TinyGPS++.h>

class GpsHandler {
public:
    void begin();
    void update();
    bool isFixed(); // Ritorna true se i satelliti sono agganciati

    // Dati di navigazione
    double getLat();
    double getLng();
    double getHdop();
    float getAlt();   // Altitudine GPS
    float getSpeed(); // in km/h

    // Dati orari (fondamentali per i log IGC)
    uint8_t getHour();
    uint8_t getMinute();
    uint8_t getSecond();
    uint16_t getYear();
    uint8_t getMonth();
    uint8_t getDay();
    uint32_t getSatellites();

    String getPendingNMEA();

private:
    TinyGPSPlus _gps;
    String _currentLine = "";
    String _pendingNMEA = "";
};

#endif