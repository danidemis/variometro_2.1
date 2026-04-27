#ifndef LOGGER_HANDLER_H
#define LOGGER_HANDLER_H

#include <Arduino.h>
#include <SD.h>
#include "StorageHandler.h"

class LoggerHandler {
public:
    void begin(StorageHandler* storage);
    
    // Avvia un nuovo file (es. passando anno, mese, giorno dal GPS)
    bool startNewTrack(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    
    // Scrive un singolo punto traccia
    void logPoint(uint8_t hour, uint8_t minute, uint8_t second, double lat, double lng, float baroAlt, float gpsAlt);
    
    void stopTrack();
    bool isLogging();

private:
    StorageHandler* _storage;
    File _logFile;
    bool _isLogging = false;
    String _currentFileName;

    // Funzione di utilità per formattare le coordinate nel formato strano dell'IGC
    String formatIgcCoordinate(double coord, bool isLat);
};

#endif