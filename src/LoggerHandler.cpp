#include "LoggerHandler.h"

void LoggerHandler::begin(StorageHandler* storage) {
    _storage = storage;
}

bool LoggerHandler::startNewTrack(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (_isLogging) stopTrack();

    // Genera il nome file basato su data E ora: es. /20260418_143005.igc
    char fileName[32];
    sprintf(fileName, "/%04d%02d%02d_%02d%02d%02d.igc", year, month, day, hour, minute, second);
    _currentFileName = String(fileName);
    
    // Controllo di sicurezza estremo (altamente improbabile che serva allo stesso secondo esatto)
    int index = 1;
    while (SD.exists(_currentFileName)) {
        sprintf(fileName, "/%04d%02d%02d_%02d%02d%02d_%d.igc", year, month, day, hour, minute, second, index);
        _currentFileName = String(fileName);
        index++;
    }

    _logFile = SD.open(_currentFileName, FILE_WRITE);
    if (!_logFile) {
        Serial.println("ERRORE: Impossibile creare il file IGC!");
        return false;
    }

    // --- SCRITTURA DEGLI HEADER IGC (Record H) ---
    _logFile.println("A SKYSkyVarioS3"); 
    
    char dateHeader[30];
    sprintf(dateHeader, "HFDTE%02d%02d%02d", day, month, year - 2000);
    _logFile.println(dateHeader);
    
    _logFile.println("HFPLTPILOTINCHARGE:" + _storage->config.pilotName);
    _logFile.println("HFGTYGLIDERTYPE:" + _storage->config.gliderType);
    _logFile.println("HFFTYFRIGID:SkyVario S3");
    _logFile.println("I023638FXA3941ENL"); 
    
    _logFile.flush(); 
    _isLogging = true;
    Serial.println("Registrazione traccia avviata: " + _currentFileName);
    return true;
}

void LoggerHandler::logPoint(uint8_t hour, uint8_t minute, uint8_t second, double lat, double lng, float baroAlt, float gpsAlt) {
    if (!_isLogging || !_logFile) return;

    // Il record B dell'IGC ha questo formato esatto:
    // B HHMMSS DDMMmmmN DDDMMmmmE A PPPPP GGGGG
    // P = Altitudine Barometrica, G = Altitudine GPS

    char timeStr[10];
    sprintf(timeStr, "%02d%02d%02d", hour, minute, second);

    String latStr = formatIgcCoordinate(lat, true);
    String lngStr = formatIgcCoordinate(lng, false);

    char altStr[20];
    // Se l'altitudine è negativa, l'IGC non vuole il meno ma lo lascia a zero o lo codifica diversamente, 
    // ma per semplicità assumiamo voli positivi o limitiamo a 0 per evitare errori di formato.
    long bAlt = max(0L, (long)baroAlt);
    long gAlt = max(0L, (long)gpsAlt);
    
    sprintf(altStr, "A%05ld%05ld", bAlt, gAlt); // A = Fix GPS Valido

    // Stringa finale per il punto traccia
    String bRecord = "B" + String(timeStr) + latStr + lngStr + String(altStr);
    
    _logFile.println(bRecord);
    _logFile.flush(); // Assicura che i dati siano scritti e non persi se la scheda si spegne
}

void LoggerHandler::stopTrack() {
    if (_isLogging && _logFile) {
        _logFile.close();
        Serial.println("Traccia IGC chiusa e salvata.");
    }
    _isLogging = false;
}

bool LoggerHandler::isLogging() {
    return _isLogging;
}

String LoggerHandler::formatIgcCoordinate(double coord, bool isLat) {
    // L'IGC vuole Gradi e Minuti Decimali (es: 45° 30.5' N -> 4530500N)
    char dir = isLat ? (coord >= 0 ? 'N' : 'S') : (coord >= 0 ? 'E' : 'W');
    coord = abs(coord);
    
    int degrees = (int)coord;
    double minutes = (coord - degrees) * 60.0;
    int minInt = (int)(minutes * 1000.0); // 3 cifre decimali dei minuti

    char buffer[20];
    if (isLat) {
        sprintf(buffer, "%02d%05d%c", degrees, minInt, dir);
    } else {
        sprintf(buffer, "%03d%05d%c", degrees, minInt, dir);
    }
    return String(buffer);
}