#include "BatteryHandler.h"
#include "Config.h"

void BatteryHandler::begin() {
    // Configura la risoluzione a 12 bit (0-4095)
    analogReadResolution(12);
    // Imposta l'attenuazione a 11dB (Permette all'ESP32 di misurare tensioni fino a circa 3.3V sul pin)
    analogSetAttenuation(ADC_11db); 
    
    update(); // Prima lettura immediata
}

void BatteryHandler::update() {
    // Legge la batteria ogni 2 secondi
    if (millis() - _lastRead > 2000) {
        // Lettura grezza del sensore (da 0 a 4095)
        int raw = analogRead(BATTERY_PIN);
        
        // Calcolo della tensione letta FISICAMENTE sul pin dell'ESP32 (massimo 3.3V)
        float pinVoltage = (raw / 4095.0) * 3.3;
        
        // Calcolo della tensione reale della batteria moltiplicando per il fattore del partitore
        _voltage = pinVoltage * ADC_MULTIPLIER;

        // Mappa la percentuale basata su una LiPo 1S (3.3V vuota - 4.2V piena)
        if (_voltage >= 4.15) _percentage = 100;
        else if (_voltage <= 3.3) _percentage = 0;
        else {
            _percentage = (int)(((_voltage - 3.3) / (4.2 - 3.3)) * 100);
        }

        // --- STAMPA DI DEBUG ---
        //Serial.printf("[DEBUG BATT] Raw: %4d | Pin: %.2fV | Batt: %.2fV | Perc: %d%%\n", 
        //              raw, pinVoltage, _voltage, _percentage);

        _lastRead = millis();
    }
}

float BatteryHandler::getVoltage() { return _voltage; }
int BatteryHandler::getPercentage() { return _percentage; }