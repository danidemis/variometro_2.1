#include <Arduino.h>
#include "Config.h"
#include "BaroHandler.h"
#include "AudioHandler.h"
#include "KalmanVario.h"
#include "BatteryHandler.h"
#include "GpsHandler.h"
#include "StorageHandler.h"
#include "BleHandler.h"
#include "WebHandler.h"
#include "LoggerHandler.h"
#include "CompassHandler.h"

BaroHandler baro;
AudioHandler audio;
KalmanVario kalman(0.005, 0.01, 1.0);
BatteryHandler battery;
GpsHandler gps;
StorageHandler storage;
BleHandler ble;
WebHandler web;
LoggerHandler logger;
CompassHandler compass;

unsigned long lastUpdate = 0;
unsigned long lastGpsDebug = 0;
unsigned long lastBleUpdate = 0;

enum SystemMode { MODE_FLIGHT, MODE_CONFIG };
SystemMode currentMode;

void testGpsHardware() {
    Serial.println("\n=====================================");
    Serial.println("   DIAGNOSTICA HARDWARE GPS AVVIATA  ");
    Serial.println("Ascolto i cavi TX/RX per 3 secondi...");
    Serial.println("=====================================\n");

    Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); 

    unsigned long startTest = millis();
    int byteCount = 0;

    while (millis() - startTest < 3000) {
        while (Serial1.available()) {
            char c = Serial1.read();
            Serial.print(c); 
            byteCount++;
        }
    }

    Serial.println("\n\n=====================================");
    if (byteCount > 0) {
        Serial.printf(">>> SUCCESSO! Ricevuti %d byte.\n", byteCount);
        Serial.println(">>> I cavi TX e RX comunicano perfettamente.");
    } else {
        Serial.println(">>> ERRORE: Silenzio assoluto sul cavo.");
    }
    Serial.println("=====================================\n");
    
    Serial1.end(); 
    delay(1000);
}

void TaskAudio(void *p) {
    for(;;) {
        audio.run(); 
    }
}

void setup() {
    Serial.begin(115200);
    delay(5000); 

    pinMode(BTN_1, INPUT_PULLUP); 
    battery.begin();
    storage.begin();

    if (digitalRead(BTN_1) == LOW) {
        currentMode = MODE_CONFIG;
        Serial.println(">>> AVVIO IN MODALITÀ CONFIGURAZIONE (WIFI) <<<");
        audio.begin();
        baro.begin();
        gps.begin();
        compass.begin();
        ble.begin(storage.config.bleName);
        web.begin(&storage, &audio, &gps, &compass, &battery);
    } else {
        currentMode = MODE_FLIGHT;
        Serial.println(">>> AVVIO IN MODALITÀ VOLO <<<");
        
        baro.begin();
        compass.begin();
        audio.begin();
        ble.begin(storage.config.bleName);
        gps.begin(); 

        baro.update();
        audio.setVolume(storage.config.varioVolume);
        kalman.setState(44330.0 * (1.0 - pow(baro.getPressure() / 1013.25, 0.190295)), 0);

        xTaskCreatePinnedToCore(TaskAudio, "Audio", 4096, NULL, 10, NULL, 1);
    }
}

void loopFlightMode() {
    // --- VARIABILI DI STATO ---
    static float lastValidAlt = 0;
    static bool firstRun = true;
    static bool sdError = false;      // Errore MicroSD
    static bool manualStart = false;  // Stato partenza manuale
    static bool wasGpsFixed = false;  // Memoria stato GPS (per il Jingle)

    static float currentAlt = 0;
    static float currentVsi = 0;

    static float elevationOffset = 0.0;
    static bool isAltCalibrated = false;

    static unsigned long lastLogUpdate = 0;
    static int landingTimer = 0; 

    static float validPressurePa = 101325.0; // Pressione filtrata per il Bluetooth
    static int spikeCount = 0;               // Valvola di sicurezza per il filtro

    // --- 1. LETTURA SENSORI BARO/AUDIO (50Hz) ---
    if (millis() - lastUpdate >= (1000 / BARO_RATE_HZ)) {
        float dt = (millis() - lastUpdate) / 1000.0f;
        lastUpdate = millis();
        
        if (baro.update() == 0) {
            float currentRawPressure = baro.getPressure();
            float rawAlt = 44330.0 * (1.0 - pow(currentRawPressure / 1013.25, 0.190295));
            
            // Logica Calibrazione Altitudine
            if (!isAltCalibrated) {
                if (!storage.config.autoElevation) {
                    elevationOffset = storage.config.manualElevation - rawAlt;
                    isAltCalibrated = true;
                    Serial.printf("Calibrazione Manuale OK: Offset %.1f m\n", elevationOffset);
                } else {
                    if (gps.isFixed() && gps.getSatellites() >= 6 && gps.getHdop() < 2.5) {
                        elevationOffset = gps.getAlt() - rawAlt;
                        isAltCalibrated = true;
                        Serial.printf("Calibrazione Auto GPS OK: Offset %.1f m (Sats: %d)\n", elevationOffset, gps.getSatellites());
                        audio.playTone(1500, 50); delay(50); audio.playTone(1500, 50); 
                    }
                }
            }

            float realAltQNH = rawAlt + elevationOffset;
            
            if (!firstRun) {
                // FILTRO ANTI-PICCO ESTREMO
                // (Saltare 2m in 20 millisecondi = cadere a 360 km/h. È un errore del sensore!)
                if (abs(rawAlt - lastValidAlt) < 2.0f || spikeCount > 10) {
                    kalman.update(realAltQNH, dt, &currentAlt, &currentVsi);
                    validPressurePa = currentRawPressure * 100.0f; // Salviamo la pressione "buona" per il BLE
                    lastValidAlt = rawAlt;  // L'ancora di sicurezza si aggiorna solo con dati sani
                    spikeCount = 0;
                } else {
                    spikeCount++; // Ignoriamo il picco, ma contiamo per evitare blocchi infiniti
                }
            } else {
                firstRun = false;
                currentAlt = realAltQNH;
                validPressurePa = currentRawPressure * 100.0f;
                lastValidAlt = rawAlt;
            }
        }

        if (abs(currentVsi) < storage.config.varioSensitivity) audio.setVSI(0);
        else audio.setVSI(currentVsi);
    }
    
    // --- 2. LETTURA BATTERIA E GPS ---
    battery.update();
    gps.update();
    compass.update();
    
    // Notifica Sonora GPS (Suona solo la prima volta che prende i satelliti)
    if (gps.isFixed() && gps.getYear() > 2000) {
        if (!wasGpsFixed) {
            wasGpsFixed = true;
            audio.playGpsFixJingle();
            Serial.println(">>> GPS FIX OK! Pronto al decollo. <<<");
        }
    } else {
        wasGpsFixed = false;
    }
    
    // --- 3. INVIO DATI A XCTRACK VIA BLE ---
    if (ble.isConnected()) {
        
        // A. Svuota il buffer del GPS NMEA
        String nmeaLine = gps.getPendingNMEA();
        while (nmeaLine.length() > 0) {
            ble.sendRaw(nmeaLine);
            nmeaLine = gps.getPendingNMEA();
            delay(2); // Piccolo respiro per non intasare la coda BLE
        }

        // B. Variometro a 10Hz (Ogni 100ms)
        if (millis() - lastBleUpdate >= 100) {
            lastBleUpdate = millis();
            ble.sendLK8EX1(
                (long)validPressurePa, 
                (long)currentAlt, 
                (long)(currentVsi * 100), 
                20, 
                battery.getPercentage()
            );
        }

        // C. Bussola a 5Hz (Ogni 200ms)
        static unsigned long lastCompassBle = 0;
        if (millis() - lastCompassBle >= 200) {
            lastCompassBle = millis();
            ble.sendCompassNMEA(compass.getHeading());
        }
    }
    
    // --- 4. DEBUG SERIALE GPS (5Hz) ---
    if (millis() - lastGpsDebug > 5000) {
        lastGpsDebug = millis();
        if (gps.isFixed()) {
            Serial.printf("GPS: Fix OK! | Sat: %d | Lat: %.5f | Lng: %.5f | Alt: %.1f m | Spd: %.1f km/h\n", 
                          gps.getSatellites(), gps.getLat(), gps.getLng(), gps.getAlt(), gps.getSpeed());
        } else {
            Serial.println("GPS: Ricerca satelliti in corso...");
        }
    }

    // --- 5. LOGICA PULSANTE (Anti-Rimbalzo) ---
    static unsigned long btnPressTime = 0;
    static bool btnPressed = false;
    static bool actionTriggered = false;
    
    static unsigned long lastDebounceTime = 0;
    static int buttonState = HIGH;
    static int lastButtonState = HIGH;
    
    int reading = digitalRead(BTN_1);

    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > 50) {
        if (reading != buttonState) {
            buttonState = reading;
        }
    }
    lastButtonState = reading;

    if (buttonState == LOW) {
        if (!btnPressed) { 
            btnPressed = true; 
            btnPressTime = millis(); 
            actionTriggered = false; 
        }
        
        if (btnPressed && !actionTriggered && (millis() - btnPressTime > 2000)) {
            actionTriggered = true;

            if (storage.config.autoLog) {
                audio.playAutoModeFeedback();
                Serial.println("Info: Decollo automatico attivo. Registro io quando ti muovi.");
            } else {
                if (!manualStart) {
                    manualStart = true;
                    audio.playStartJingle();
                    Serial.println("Manuale: Volo avviato dal pilota.");
                } else {
                    manualStart = false;
                    if (logger.isLogging()) logger.stopTrack();
                    audio.playStopJingle();
                    Serial.println("Manuale: Volo fermato dal pilota.");
                }
            }
        }
    } else {
        btnPressed = false;
        actionTriggered = false;
    }

    // --- 6. DATA LOGGING IGC E AUTO-DECOLLO (1 Hz) ---
    if (millis() - lastLogUpdate >= 1000) {
        lastLogUpdate = millis();
        bool shouldBeLogging = false;

        // Caso A: Decollo Automatico
        if (storage.config.autoLog && !sdError) {
            if (!logger.isLogging()) {
                if (gps.isFixed() && gps.getSatellites() >= 4 && (gps.getSpeed() > 10.0 || currentVsi > 1.5)) {
                    shouldBeLogging = true;
                }
            } else {
                if (gps.getSpeed() < 5.0 && abs(currentVsi) < 0.5) {
                    landingTimer++;
                    if (landingTimer > 45) {
                        logger.stopTrack();
                        audio.playStopJingle();
                        landingTimer = 0;
                    }
                } else { landingTimer = 0; }
            }
        } 
        // Caso B: Partenza Manuale
        else if (manualStart && !sdError) {
            shouldBeLogging = true;
        }

        // Esecuzione Logging
        if (shouldBeLogging && !logger.isLogging() && gps.isFixed() && gps.getYear() > 2000) {
            logger.begin(&storage);
            bool success = logger.startNewTrack(
                gps.getYear(), gps.getMonth(), gps.getDay(), 
                gps.getHour(), gps.getMinute(), gps.getSecond()
            );
            
            if (success) {
                if (storage.config.autoLog) audio.playStartJingle(); 
            } else {
                sdError = true;       
                manualStart = false;  
                audio.playErrorSound(); 
                Serial.println("ERRORE CRITICO: MicroSD guasta o assente. Logging disabilitato.");
            }
        }

        // Scrittura Punti Traccia (Sottraiamo l'offset per rispettare lo Standard IGC QNE)
        if (logger.isLogging()) {
            logger.logPoint(gps.getHour(), gps.getMinute(), gps.getSecond(), gps.getLat(), gps.getLng(), (currentAlt - elevationOffset), gps.getAlt());
        }
    }
}

void loopConfigMode() {
    static unsigned long lastDiagBle = 0;
    
    audio.loopConfig();
    gps.update();      // Fondamentale: legge i dati dal GPS anche in Config
    battery.update();  // Aggiorna la percentuale batteria
    compass.update(); // Decommenta quando la bussola sarà attiva
    // --- SCATOLA NERA BLUETOOTH (Ogni 3 secondi) ---
    if (ble.isConnected() && (millis() - lastDiagBle > 3000)) {
        lastDiagBle = millis();
        
        char diagMsg[128];
        sprintf(diagMsg, "DIAG > Sat:%d | HDOP:%.1f | AltGPS:%.1f | Batt:%d%% | Head:%d\r\n", 
                gps.getSatellites(), gps.getHdop(), gps.getAlt(), 
                battery.getPercentage(), compass.getHeading());
        
        ble.sendRaw(String(diagMsg));
    }
}

void loop() {
    if (currentMode == MODE_FLIGHT) {
        loopFlightMode();
    } else {
        loopConfigMode();
    }
}