#ifndef STORAGE_HANDLER_H
#define STORAGE_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

struct DeviceConfig {
    // Tab 1: Anagrafica & Connettività
    String pilotName = "Mario Rossi";
    String gliderType = "Parapendio";
    String wifiSSID = "SkyVario-S3";
    String wifiPass = "12345678";
    String bleName = "SkyVario-S3";
    
    
    // Tab 2: Variometro
    int varioVolume = 80;         // Percentuale volume 0-100
    float varioSensitivity = 0.20; // Zona morta in m/s (es. non suona sotto i 0.2 m/s)
    bool autoLog = true; // Di default attiviamo il decollo automatico
    bool autoElevation = true; 
    int manualElevation = 0; // Altitudine del decollo in metri
};

class StorageHandler {
public:
    bool begin();
    bool loadConfig();
    bool saveConfig();
    
    DeviceConfig config; 

private:
    SPIClass _spi = SPIClass(FSPI); 
};

#endif