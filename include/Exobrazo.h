#ifndef EXOBRAZO_H
#define EXOBRAZO_H

#include "Motor.h"
#include "Signal.h"
#include "GSignal.h"
#include "MonitorSerial.h"
#include "defines.h"

class Exobrazo {
public:
    // --- 1. Componentes Públicos ---
    MonitorSerial monitor;
    Motor* motores[3]; // Array de punteros

private:
    // --- 2. Componentes Privados (en orden) ---
    Signal signalMotor1;
    Signal signalMotor2;
    Signal signalMotor3;
    
    Motor motor1;
    Motor motor2;
    Motor motor3;
    
    GSignal senalGeneral;

    // --- 3. Estado de la Aplicación (en orden) ---
    enum class ModoControl { PRUEBA, WEB };
    ModoControl modoActual;
    
    int dirEstadosPrueba[3];

public:
    Exobrazo();
    void iniciar();
    void ejecutarCicloPrincipal();

    // --- API de Control (para la Web) ---
    void moverMotorWeb(int motorIndex, float aceleracion, uint32_t direccion, int pasos);
    void setModoPrueba();
};

#endif // EXOBRAZO_H