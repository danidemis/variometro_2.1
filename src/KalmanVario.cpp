#include "KalmanVario.h"
#include <Arduino.h>

KalmanVario::KalmanVario(float q_alt, float q_vsi, float r_alt) 
    : _q_alt(q_alt), _q_vsi(q_vsi), _r_alt(r_alt) {}

void KalmanVario::update(float z_measured, float dt, float* alt_est, float* vsi_est) {
    // PROTEZIONE: Se l'ingresso è NaN, non fare nulla
    if (isnan(z_measured)) return;

    // 1. Predizione
    _alt = _alt + (_vsi * dt);
    
    _p11 += dt * (_p12 + _p21 + dt * _p22) + _q_alt;
    _p12 += dt * _p22;
    _p21 += dt * _p22;
    _p22 += _q_vsi;

    // 2. Aggiornamento (Innovation)
    float y = z_measured - _alt;
    float s = _p11 + _r_alt;
    if (s == 0) return; // Evita divisione per zero

    float k1 = _p11 / s;
    float k2 = _p21 / s;

    _alt += k1 * y;
    _vsi += k2 * y;

    // Salviamo i vecchi valori per l'aggiornamento corretto della matrice P
    float p11_old = _p11;
    float p12_old = _p12;

    _p11 -= k1 * p11_old;
    _p12 -= k1 * p12_old;
    _p21 -= k2 * p11_old;
    _p22 -= k2 * p12_old;

    *alt_est = _alt;
    *vsi_est = _vsi;
}