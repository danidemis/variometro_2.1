#include "BleHandler.h"
#include <BLE2902.h> // <--- FONDAMENTALE PER EVITARE IL GATT 147

// UUID standard per il Nordic UART Service (NUS)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void BleHandler::begin(String deviceName) {
    BLEDevice::init(deviceName.c_str());
    
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(this);

    BLEService *pService = _pServer->createService(SERVICE_UUID);

    // Caratteristica TX (L'ESP32 invia dati al telefono)
    // Aggiunto PROPERTY_READ per compatibilità con alcuni stack Android
    _pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    _pTxCharacteristic->addDescriptor(new BLE2902());

    // Caratteristica RX (Il telefono invia dati all'ESP32)
    // Aggiunto WRITE_NR (No Response) richiesto dal protocollo NUS
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );

    pService->start();

    // --- FIX PER ANDROID E XCTRACK ---
    // Forziamo l'ESP32 ad annunciare chiaramente quale servizio offre
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Funzioni per evitare problemi con iOS/Android
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE Avviato. In attesa di connessione (Nordic UART)...");
}

bool BleHandler::isConnected() {
    return _deviceConnected;
}

void BleHandler::onConnect(BLEServer* pServer) {
    _deviceConnected = true;
    Serial.println("BLE: Dispositivo connesso!");
}

void BleHandler::onDisconnect(BLEServer* pServer) {
    _deviceConnected = false;
    Serial.println("BLE: Dispositivo disconnesso. Riavvio Advertising...");
    delay(500); 
    pServer->getAdvertising()->start();
}

void BleHandler::sendLK8EX1(long pressure_Pa, long alt_m, long vsi_cms, int temp_c, int battery_percent) {
    if (!_deviceConnected) return;

    char buffer[100];
    sprintf(buffer, "LK8EX1,%ld,%ld,%ld,%d,%d,", pressure_Pa, alt_m, vsi_cms, temp_c, battery_percent);
    
    String sentence = String(buffer);
    String checksum = calculateChecksum(sentence);
    String finalSentence = "$" + sentence + "*" + checksum + "\r\n";

    _pTxCharacteristic->setValue(finalSentence.c_str());
    _pTxCharacteristic->notify();
}

String BleHandler::calculateChecksum(String sentence) {
    uint8_t checksum = 0;
    for (int i = 0; i < sentence.length(); i++) {
        checksum ^= sentence[i];
    }
    char hexChecksum[3];
    sprintf(hexChecksum, "%02X", checksum);
    return String(hexChecksum);
}

void BleHandler::sendRaw(String data) {
    if (!_deviceConnected || data.length() == 0) return;
    _pTxCharacteristic->setValue(data.c_str());
    _pTxCharacteristic->notify();
}

void BleHandler::sendCompassNMEA(int heading) {
    if (!_deviceConnected) return;

    // Formato NMEA: $HCHDG,Gradi Magnetici,,,,*Checksum
    char buffer[32];
    sprintf(buffer, "HCHDG,%d.0,,,,", heading); 
    
    String sentence = String(buffer);
    String checksum = calculateChecksum(sentence);
    String finalSentence = "$" + sentence + "*" + checksum + "\r\n";

    _pTxCharacteristic->setValue(finalSentence.c_str());
    _pTxCharacteristic->notify();
}