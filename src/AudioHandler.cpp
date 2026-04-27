#include "AudioHandler.h"
#include "Config.h"

bool AudioHandler::begin() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK, .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DIN, .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    return true;
}

void AudioHandler::setVolume(int volume) {
    _volume = volume;
}

void AudioHandler::setVSI(float vsi) { _vsi = vsi; }

void AudioHandler::run() {
    // SEMAFORO: Se un jingle o un bip di sistema è attivo, mettiti in pausa
    if (_isJingleActive) {
        vTaskDelay(pdMS_TO_TICKS(10)); 
        return;
    }
    
    unsigned long elapsed = millis() - _lastToggle;
    bool readyForNext = false;

    if (_state == SILENCE) {
        readyForNext = true;
    } else if (_state == SINK) {
        if (elapsed > 100) readyForNext = true; 
    } else if (_state == CLIMB) {
        if (elapsed >= (unsigned long)_currentPeriod) readyForNext = true;
    }

    if (readyForNext) {
        _lastToggle = millis();
        elapsed = 0;

        if (_vsi > 0.4) {
            _state = CLIMB;
            int rawFreq = 440 + (pow(_vsi, 1.3) * 180);
            _currentFreq = (rawFreq / 20) * 20; 
            if (_currentFreq > 2200) _currentFreq = 2200;

            _currentPeriod = 450 / (1.0 + _vsi * 0.6);
            if (_currentPeriod < 80) _currentPeriod = 80;
            
            _beepDuration = _currentPeriod * 0.55; 
            
        } else if (_vsi < -1.5) {
            _state = SINK;
            int rawFreq = 400 + (_vsi * 40); 
            _currentFreq = (rawFreq / 10) * 10;
            if (_currentFreq < 100) _currentFreq = 100;
            
        } else {
            _state = SILENCE;
        }
    }

    int amplitude = map(_volume, 0, 100, 0, 15000);

    if (_state == CLIMB) {
        if (elapsed < (unsigned long)_beepDuration) {
            playRaw(_currentFreq, amplitude); 
        } else {
            playSilence();
        }
    } else if (_state == SINK) {
        playRaw(_currentFreq, amplitude * 0.8); 
    } else {
        playSilence();
    }
}

void AudioHandler::playRaw(int freq, int volume) {
    size_t bytes_written;
    int16_t samples[64];
    float phase_inc = (2.0 * PI * freq) / SAMPLE_RATE;
    for (int i = 0; i < 64; i++) {
        samples[i] = (int16_t)(sin(_phase) * volume);
        _phase += phase_inc;
        if (_phase >= 2.0 * PI) _phase -= 2.0 * PI;
    }
    i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
}

void AudioHandler::playSilence() {
    size_t bytes_written;
    int16_t samples[64] = {0};
    i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
}

void AudioHandler::triggerTestBeep(int volume) {
    _isJingleActive = true; 
    _volume = volume;       
    playTone(1000, 100);
    for(int i=0; i<10; i++) playSilence();
    _isJingleActive = false; 
}

void AudioHandler::loopConfig() {
    if (_testBeepActive) {
        if (millis() - _testBeepStart < 500) {
            int amplitude = map(_testBeepVolume, 0, 100, 0, 15000);
            playRaw(800, amplitude); 
        } else {
            _testBeepActive = false;
            playSilence(); 
        }
    } else {
        playSilence();
    }
}

// --- LA FUNZIONE FIXATA E BLINDATA ---
void AudioHandler::playTone(int freq, int duration_ms) {
    // 1. Memorizza lo stato attuale del semaforo e lo alza (THREAD-SAFE)
    bool wasActive = _isJingleActive;
    _isJingleActive = true; 
    
    int amplitude = (_volume * _volume * 15000) / 10000; 
    unsigned long start = millis();
    while (millis() - start < (unsigned long)duration_ms) {
        playRaw(freq, amplitude);
    }
    
    for(int i=0; i<10; i++) {
        playSilence();
    }
    
    // 2. Ripristina il semaforo allo stato in cui lo aveva trovato
    _isJingleActive = wasActive; 
}

void AudioHandler::playStartJingle() {
    _isJingleActive = true; 
    playTone(440, 150); playTone(554, 150); playTone(659, 300); 
    _isJingleActive = false; 
}

void AudioHandler::playStopJingle() {
    _isJingleActive = true;
    playTone(659, 150); playTone(554, 150); playTone(440, 300); 
    _isJingleActive = false;
}

void AudioHandler::playAutoModeFeedback() {
    _isJingleActive = true;
    for(int i=0; i<3; i++) {
        playTone(1000, 80); 
        delay(50); 
    }
    _isJingleActive = false;
}

void AudioHandler::playGpsFixJingle() {
    _isJingleActive = true;
    playTone(880, 100); playTone(1046, 200); 
    _isJingleActive = false;
}

void AudioHandler::playErrorSound() {
    _isJingleActive = true;
    playTone(200, 500);
    _isJingleActive = false;
}