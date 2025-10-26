#include "Exobrazo.h"

Exobrazo::Exobrazo() :
    // --- ORDEN DE INICIALIZACIÓN CORREGIDO ---

    // 1. Componentes Públicos (en orden de .h)
    monitor(),

    // 2. Componentes Privados (en orden de .h)
    signalMotor1(SIGNAL_PIN_1, ACCEL_PRUEBA_STD, PASOS_PRUEBA_STD),
    signalMotor2(SIGNAL_PIN_2, ACCEL_PRUEBA_STD, PASOS_PRUEBA_STD),
    signalMotor3(SIGNAL_PIN_3, ACCEL_PRUEBA_STD, PASOS_PRUEBA_STD),

    motor1(STEP_PIN_1, DIR_PIN_1, (adc1_channel_t)ADC_CHANNEL_7, signalMotor1, 
           ACCEL_MAX_M1, VEL_MAX_M1, PASOS_REV_M1, LIM_MIN_M1, LIM_MAX_M1),
           
    motor2(STEP_PIN_2, DIR_PIN_2, (adc1_channel_t)ADC_CHANNEL_4, signalMotor2, 
           ACCEL_MAX_M2, VEL_MAX_M2, PASOS_REV_M2, LIM_MIN_M2, LIM_MAX_M2),
           
    motor3(STEP_PIN_3, DIR_PIN_3, (adc1_channel_t)ADC_CHANNEL_5, signalMotor3, 
           ACCEL_MAX_M3, VEL_MAX_M3, PASOS_REV_M3, LIM_MIN_M3, LIM_MAX_M3),

    senalGeneral(GSIGNAL_PIN),

    // 3. Estado de la Aplicación (en orden de .h)
    modoActual(ModoControl::PRUEBA)
{
    // --- Inicialización en el cuerpo (para arrays) ---
    
    motores[0] = &motor1;
    motores[1] = &motor2;
    motores[2] = &motor3;

    dirEstadosPrueba[0] = 1;
    dirEstadosPrueba[1] = 1;
    dirEstadosPrueba[2] = 1;
}

void Exobrazo::iniciar() {
    // Vincular las 3 señales de los motores al control del botón general
    senalGeneral.agregarSignal(&signalMotor1);
    senalGeneral.agregarSignal(&signalMotor2);
    senalGeneral.agregarSignal(&signalMotor3); // Asumiendo que es signalMotor3
}

void Exobrazo::ejecutarCicloPrincipal() {
    
    // 1. Siempre actualizar la posición de todos los motores
    for (int i = 0; i < 3; i++) {
        // Corrección: Usamos el método que sí existe
        motores[i]->leerPosicion(); 
    }

    // 2. SI ESTAMOS EN MODO WEB, NO ESCUCHAR BOTONES.
    if (modoActual == ModoControl::WEB) {
        return; 
    }

    // 3. (MODO PRUEBA) Actualizar el estado del botón general
    senalGeneral.actualizarEstado();

    // 4. (MODO PRUEBA) Iterar sobre cada motor
    for (int i = 0; i < 3; i++) {

        // 5. Comprobar si la señal de este motor está activa
        if (motores[i]->signal.obtenerEstado()) {

            // 6. LÓGICA DE REBOTE
            float posActual = motores[i]->getPosicionActual(); // Leemos el valor ya actualizado
            float limMin = motores[i]->getLimiteMin();
            float limMax = motores[i]->getLimiteMax();

            // Lógica de límites invertidos (Motor 3)
            if (limMin > limMax) {
                if (posActual >= limMin) {
                    dirEstadosPrueba[i] = -1;
                } else if (posActual <= limMax) {
                    dirEstadosPrueba[i] = 1;
                }
            } else { // Lógica normal
                if (posActual >= limMax) {
                    dirEstadosPrueba[i] = -1;
                } else if (posActual <= limMin) {
                    dirEstadosPrueba[i] = 1;
                }
            }

            // 7. DAR LA ORDEN
            uint32_t dir_gpio = (dirEstadosPrueba[i] == 1) ? 1 : 0; 
            
            motores[i]->activarMovimiento(
                ACCEL_PRUEBA_STD, 
                dir_gpio, 
                PASOS_PRUEBA_STD
            );
        }
    } 
}

// --- API DE CONTROL (HUECOS PARA LA WEB) ---

void Exobrazo::moverMotorWeb(int motorIndex, float aceleracion, uint32_t direccion, int pasos) {
    this->modoActual = ModoControl::WEB;
    if (motorIndex < 0 || motorIndex > 2) return;
    motores[motorIndex]->activarMovimiento(aceleracion, direccion, pasos);
}

void Exobrazo::setModoPrueba() {
    this->modoActual = ModoControl::PRUEBA;
}