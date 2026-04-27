#ifndef CONFIG_H
#define CONFIG_H

// I2C Pins (Sensore MS5611)
#define I2C_SDA          8 
#define I2C_SCL          9 

// I2S Pins (DAC MAX98357A)
#define I2S_BCLK         21 
#define I2S_LRC          47 
#define I2S_DIN          14 

// Parametri di Test
#define I2C_FREQ         100000 // Iniziamo a 100kHz per stabilità
#define SAMPLE_RATE      16000

// --- UART (GPS Flywoo GM10 Pro) ---

#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 115200 // Spesso 9600 o 115200 per M10

// --- SPI microSD (Slot SD) ---

#define SD_SCK 12
#define SD_MISO 13
#define SD_MOSI 11
#define SD_CS 10



// --- ANALOG & BUTTONS ---

#define BATTERY_PIN 4
#define BTN_1 1
#define BTN_2 2

// --- COSTANTI DI SISTEMA ---

#define BARO_RATE_HZ 50 // Lettura barometro ogni 20ms
#define LOG_RATE_SEC 1 // Un punto nel log ogni secondo
#define DEVICE_NAME "SkyVario-S3"

#endif