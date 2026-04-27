#include "GpsHandler.h"
#include "Config.h"

// RIMOSSO: HardwareSerial gpsSerial(1); non serve, usiamo direttamente Serial1 nativa!

void GpsHandler::begin() {
    // Configura la Seriale 1 con i parametri dal Config.h
    Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("Modulo GPS Inizializzato su Serial1.");
}

void GpsHandler::update() {
    while (Serial1.available() > 0) {
        char c = Serial1.read();
        _gps.encode(c); // Continua a decodificare per il nostro codice (Kalman, IGC, ecc)
        
        // --- INTERCETTAZIONE NMEA PER BLUETOOTH ---
        _currentLine += c;
        
        // Quando la riga finisce (carattere a capo '\n')
        if (c == '\n') {
            // Selezioniamo solo le stringhe utili alla navigazione (Posizione e Velocità)
            if (_currentLine.startsWith("$GNGGA") || _currentLine.startsWith("$GPGGA") ||
                _currentLine.startsWith("$GNRMC") || _currentLine.startsWith("$GPRMC")) {
                
                _pendingNMEA += _currentLine; // Le accodiamo nel buffer
                
                // --- FIX MEMORY LEAK ---
                // Se il buffer diventa troppo grande (es. telefono non collegato),
                // teniamo solo l'ultima stringa per non far esplodere la memoria RAM dell'ESP32
                if (_pendingNMEA.length() > 500) {
                    _pendingNMEA = _currentLine; 
                }
            }
            _currentLine = ""; // Resettiamo la riga per leggere la successiva
        }
    }
}

bool GpsHandler::isFixed() {
    return _gps.location.isValid() && _gps.date.isValid(); // Più sicuro per l'IGC
}

String GpsHandler::getPendingNMEA() {
    int newlineIdx = _pendingNMEA.indexOf('\n');
    if (newlineIdx >= 0) {
        String line = _pendingNMEA.substring(0, newlineIdx + 1);
        _pendingNMEA = _pendingNMEA.substring(newlineIdx + 1);
        return line;
    }
    return "";
}

double GpsHandler::getLat() { return _gps.location.lat(); }
double GpsHandler::getLng() { return _gps.location.lng(); }
double GpsHandler::getHdop() { return _gps.hdop.hdop(); }
float GpsHandler::getAlt() { return _gps.altitude.meters(); }
float GpsHandler::getSpeed() { return _gps.speed.kmph(); }

uint8_t GpsHandler::getHour() { return _gps.time.hour(); }
uint8_t GpsHandler::getMinute() { return _gps.time.minute(); }
uint8_t GpsHandler::getSecond() { return _gps.time.second(); }
uint16_t GpsHandler::getYear() { return _gps.date.year(); }
uint8_t GpsHandler::getMonth() { return _gps.date.month(); }
uint8_t GpsHandler::getDay() { return _gps.date.day(); }
uint32_t GpsHandler::getSatellites() { return _gps.satellites.value(); }