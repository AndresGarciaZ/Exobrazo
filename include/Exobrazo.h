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

// --- 3. Tipo de Estado Público ---
// El TIPO 'ModoControl' es público para que main.cpp lo entienda
enum class ModoControl { PRUEBA, WEB };

private:
// --- 2. Componentes Privados (en orden) ---
 Signal signalMotor1;
 Signal signalMotor2;
 Signal signalMotor3;
 
 Motor motor1;
 Motor motor2;
 Motor motor3;
 
 GSignal senalGeneral;
 adc1_channel_t adc_pins[3];

// --- 3. Estado Privado de la Aplicación ---
// La VARIABLE 'modoActual' es privada
ModoControl modoActual;
int dirEstadosPrueba[3];

public:

// --- 4. Métodos Públicos (Constructor y API) ---
Exobrazo();
void iniciar();
void ejecutarCicloPrincipal();

// --- API de Control (para la Web) ---
void setModoWeb();

void moverMotorWeb(int motorIndex, float aceleracion, uint32_t direccion, int pasos);
void setModoPrueba();

//getter modoActual
ModoControl getModoActual() const;

};

#endif // EXOBRAZO_H