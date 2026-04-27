// src/AudioHandler.h
#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include "driver/i2s.h"
#include <Arduino.h>

class AudioHandler {
public:
    bool begin();
    void setVSI(float vsi); 
    void setVolume(int volume);
    void run();            
    
    // --- NUOVE FUNZIONI PER IL TEST AUDIO ---
    void triggerTestBeep(int volume); 
    void loopConfig();

    void playTone(int freq, int duration_ms);
    void playStartJingle();
    void playStopJingle();
    void playGpsFixJingle();
    void playAutoModeFeedback(); // 3 bip veloci
    void playErrorSound();
    
private:
    void playRaw(int freq, int volume);
    void playSilence();
    
    float _vsi = 0;
    float _phase = 0;
    unsigned long _lastToggle = 0;
    
    int _currentFreq = 0;
    int _currentPeriod = 0;
    int _beepDuration = 0;
    
    int _volume = 80; 
    
    enum VarioState { SILENCE, CLIMB, SINK };
    VarioState _state = SILENCE;

    // --- VARIABILI TEST AUDIO ---
    bool _testBeepActive = false;
    unsigned long _testBeepStart = 0;
    int _testBeepVolume = 0;

    volatile bool _isJingleActive = false;
};

#endif