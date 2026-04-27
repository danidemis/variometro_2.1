#include "CompassHandler.h"
#include <math.h>

#define IST8310_ADDR 0x0C

void CompassHandler::begin() {
    // 1. Soft Reset
    Wire.beginTransmission(IST8310_ADDR);
    Wire.write(0x0B); // CNTL2
    Wire.write(0x01); // Esegui Reset
    Wire.endTransmission();
    delay(20);
    
    // 2. Ottimizzazione delle performance e disattivazione interrupt
    Wire.beginTransmission(IST8310_ADDR);
    Wire.write(0x42); // PDCNTL
    Wire.write(0xC0);
    Wire.endTransmission();
    
    // 3. Noise filter (Media interna x16)
    Wire.beginTransmission(IST8310_ADDR);
    Wire.write(0x41); // AVGCNTL
    Wire.write(0x24);
    Wire.endTransmission();

    Serial.println("Bussola IST8310 inizializzata con successo su 0x0C!");
}

void CompassHandler::update() {
    static unsigned long lastTrigger = 0;
    static bool isWaiting = false;

    if (!isWaiting) {
        // FASE 1: Ordina al sensore di effettuare una singola misurazione
        Wire.beginTransmission(IST8310_ADDR);
        Wire.write(0x0A); // Registro CNTL1
        Wire.write(0x01); // Single Measurement Mode
        Wire.endTransmission();
        
        lastTrigger = millis();
        isWaiting = true; // Cambia stato: ora siamo in attesa
    } else {
        // FASE 2: Sono passati almeno 10ms? Il calcolo è pronto!
        if (millis() - lastTrigger >= 10) {
            
            Wire.beginTransmission(IST8310_ADDR);
            Wire.write(0x03); // Indirizzo di partenza (DATA X_L)
            Wire.endTransmission(false); // FONDAMENTALE: Tieni la linea aperta!
            
            // Ritiriamo i 6 byte
            Wire.requestFrom((uint16_t)IST8310_ADDR, (uint8_t)6);
            if (Wire.available() >= 6) {
                // Formato Little Endian (prima il byte basso, poi quello alto)
                int16_t x = Wire.read() | (Wire.read() << 8);
                int16_t y = Wire.read() | (Wire.read() << 8);
                int16_t z = Wire.read() | (Wire.read() << 8); // Z non ci serve per l'azimut, ma va letto per svuotare il buffer
                
                // Applichiamo un filtro passa-basso per rendere l'ago della bussola super stabile
                _smoothX = (_smoothX * 0.85) + (x * 0.15);
                _smoothY = (_smoothY * 0.85) + (y * 0.15);
                
                // Calcolo dell'Azimut in gradi tramite trigonometria
                float rawHeading = atan2(_smoothY, _smoothX) * 180.0 / PI;
                if (rawHeading < 0) rawHeading += 360.0;
                
                _heading = (int)rawHeading;
            }
            // Finito! Resettiamo lo stato per iniziare una nuova misurazione al prossimo giro
            isWaiting = false; 
        }
    }
}

int CompassHandler::getHeading() {
    return _heading;
}