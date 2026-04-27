#include "StorageHandler.h"
#include "Config.h"

bool StorageHandler::begin() {
    // Configura i pin del bus SPI per la SD
    _spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    // Inizializza la SD a 4MHz (velocità standard e stabile)
    if (!SD.begin(SD_CS, _spi, 4000000U)) {
        Serial.println("ERRORE: MicroSD non trovata o danneggiata.");
        return false;
    }
    
    Serial.println("MicroSD inizializzata con successo!");
    
    // Prova a caricare la configurazione. Se non esiste, crea il file di default.
    if (!loadConfig()) {
        Serial.println("File config.json non trovato. Creazione file di default...");
        saveConfig();
    }
    
    return true;
}

bool StorageHandler::loadConfig() {
    if (!SD.exists("/config.json")) return false;
    File file = SD.open("/config.json", FILE_READ);
    if (!file) return false;

    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) return false;

    config.pilotName = doc["pilotName"] | "Monkey D. Luffy";
    config.gliderType = doc["gliderType"] | "Parapendio";
    config.wifiSSID = doc["wifiSSID"] | "SkyVario-Config";
    config.wifiPass = doc["wifiPass"] | "12345678";
    config.bleName = doc["bleName"] | "SkyVario-S3";
    
    config.varioVolume = doc["varioVolume"] | 80;
    config.varioSensitivity = doc["varioSensitivity"] | 0.20;

    config.autoElevation = doc["autoElevation"] | true;
    config.manualElevation = doc["manualElevation"] | 0;

    Serial.println("Configurazione caricata.");
    config.autoLog = doc["autoLog"] | true;
    return true;
}

bool StorageHandler::saveConfig() {
    File file = SD.open("/config.json", FILE_WRITE);
    if (!file) return false;

    JsonDocument doc;
    doc["pilotName"] = config.pilotName;
    doc["gliderType"] = config.gliderType;
    doc["wifiSSID"] = config.wifiSSID;
    doc["wifiPass"] = config.wifiPass;
    doc["bleName"] = config.bleName;
    
    doc["varioVolume"] = config.varioVolume;
    doc["varioSensitivity"] = config.varioSensitivity;
    doc["autoLog"] = config.autoLog;

    doc["autoElevation"] = config.autoElevation;
    doc["manualElevation"] = config.manualElevation;

    if (serializeJsonPretty(doc, file) == 0) { 
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("Configurazione salvata con successo.");
    return true;
}