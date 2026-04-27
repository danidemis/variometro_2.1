#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class BleHandler : public BLEServerCallbacks {
public:
    void begin(String deviceName);
    void sendLK8EX1(long pressure_Pa, long alt_m, long vsi_cms, int temp_c, int battery_percent);
    void sendRaw(String data);
    bool isConnected();

    // Callbacks interne per connessione/disconnessione
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

    void sendHeading(int heading);
    void sendCompassNMEA(int heading);

private:
    BLEServer* _pServer = nullptr;
    BLECharacteristic* _pTxCharacteristic = nullptr;
    bool _deviceConnected = false;

    String calculateChecksum(String sentence);
};

#endif