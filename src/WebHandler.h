#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "StorageHandler.h"
#include "AudioHandler.h"
#include "GpsHandler.h"      // <-- NUOVO
#include "CompassHandler.h"  // <-- NUOVO
#include "BatteryHandler.h"  // <-- NUOVO

class WebHandler {
public:
    void begin(StorageHandler* storage, AudioHandler* audio, GpsHandler* gps, CompassHandler* compass, BatteryHandler* battery);

private:
    StorageHandler* _storage;
    AudioHandler* _audio;
    GpsHandler* _gps;         // <-- NUOVO
    CompassHandler* _compass; // <-- NUOVO
    BatteryHandler* _battery; // <-- NUOVO

    // La pagina HTML salvata nella memoria flash
    static const char index_html[]; 
};

#endif