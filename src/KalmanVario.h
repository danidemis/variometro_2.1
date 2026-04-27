#ifndef KALMAN_VARIO_H
#define KALMAN_VARIO_H

class KalmanVario {
public:
    KalmanVario(float q_alt, float q_vsi, float r_alt);
    void update(float z_measured, float dt, float* alt_est, float* vsi_est);
    void setParams(float q_vsi, float r_alt) {
    _q_vsi = q_vsi;
    _r_alt = r_alt;
    }
    void setState(float alt, float vsi) {
        _alt = alt;
        _vsi = vsi;
        _p11 = 1; _p12 = 0; _p21 = 0; _p22 = 1; // Resetta la matrice di covarianza
    }

private:
    // Matrici del filtro (semplificate per 2D)
    float _alt = 0;
    float _vsi = 0;
    float _p11 = 1, _p12 = 0, _p21 = 0, _p22 = 1;
    float _q_alt, _q_vsi, _r_alt;
};

#endif