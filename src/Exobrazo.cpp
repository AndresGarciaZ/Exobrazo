#include "Exobrazo.h"

// --- CONSTRUCTOR CORREGIDO ---
Exobrazo::Exobrazo() :
    monitor(),

    // ¡CORRECCIÓN AQUÍ!
    // Pasamos '0' como el número de pasos, ya que
    // ejecutarCicloPrincipal calculará los suyos propios.
    signalMotor1(SIGNAL_PIN_1, ACCEL_PRUEBA_STD, 0 /*PASOS_PRUEBA_STD*/),
    signalMotor2(SIGNAL_PIN_2, ACCEL_PRUEBA_STD, 0 /*PASOS_PRUEBA_STD*/),
    signalMotor3(SIGNAL_PIN_3, ACCEL_PRUEBA_STD, 0 /*PASOS_PRUEBA_STD*/),

    // El resto de la inicialización está bien
    motor1(STEP_PIN_1, DIR_PIN_1, (adc1_channel_t)ADC_CHANNEL_7, signalMotor1, 
           ACCEL_MAX_M1, VEL_MAX_M1, PASOS_REV_M1, LIM_MIN_M1, LIM_MAX_M1),
    motor2(STEP_PIN_2, DIR_PIN_2, (adc1_channel_t)ADC_CHANNEL_4, signalMotor2, 
           ACCEL_MAX_M2, VEL_MAX_M2, PASOS_REV_M2, LIM_MIN_M2, LIM_MAX_M2),
    motor3(STEP_PIN_3, DIR_PIN_3, (adc1_channel_t)ADC_CHANNEL_5, signalMotor3, 
           ACCEL_MAX_M3, VEL_MAX_M3, PASOS_REV_M3, LIM_MIN_M3, LIM_MAX_M3),
    senalGeneral(GSIGNAL_PIN),
    modoActual(ModoControl::PRUEBA)
{
    motores[0] = &motor1;
    motores[1] = &motor2;
    motores[2] = &motor3;
    dirEstadosPrueba[0] = 1;
    dirEstadosPrueba[1] = 1;
    dirEstadosPrueba[2] = 1;
}

// iniciar() (sin cambios)
void Exobrazo::iniciar() {
    senalGeneral.agregarSignal(&signalMotor1);
    senalGeneral.agregarSignal(&signalMotor2);
    senalGeneral.agregarSignal(&signalMotor3);
}


// --- ejecutarCicloPrincipal() (MODIFICADO) ---
void Exobrazo::ejecutarCicloPrincipal() {
    
    // 1. Siempre actualizar la posición
    for (int i = 0; i < 3; i++) {
        motores[i]->leerPosicion(); 
    }

    // 2. Ignorar botones si estamos en modo Web
    if (modoActual == ModoControl::WEB) {
        return; 
    }

    // 3. Actualizar el estado del botón general
    senalGeneral.actualizarEstado();

    // 4. Iterar sobre cada motor para la prueba de rebote
    for (int i = 0; i < 3; i++) {

        // 5. Comprobar si la señal de este motor está activa
        if (motores[i]->signal.obtenerEstado()) {

            // 6. LÓGICA DE REBOTE (lee la variable actualizada en el paso 1)
            float posActual = motores[i]->getPosicionActual();
            float limMin = motores[i]->getLimiteMin();
            float limMax = motores[i]->getLimiteMax();

            // Lógica de límites invertidos (Motor 3)
            if (limMin > limMax) {
                if (posActual >= limMin) dirEstadosPrueba[i] = -1;
                else if (posActual <= limMax) dirEstadosPrueba[i] = 1;
            } else { // Lógica normal
                if (posActual >= limMax) dirEstadosPrueba[i] = -1;
                else if (posActual <= limMin) dirEstadosPrueba[i] = 1;
            }

            // Obtenemos los pasos por revolución de este motor específico
            int pasos_por_rev = motores[i]->getPasosPorRev();
            
            // Calculamos sus pasos por grado
            float steps_per_degree = (float)pasos_por_rev / 360.0f;
            
            // Calculamos cuántos pasos son el ángulo estándar (de defines.h)
            int pasos_a_mover = (int)(ANGULO_PRUEBA_STD * steps_per_degree);

            // 8. DAR LA ORDEN
            uint32_t dir_gpio = (dirEstadosPrueba[i] == 1) ? 1 : 0; 
            
            motores[i]->activarMovimiento(
                ACCEL_PRUEBA_STD, // Aceleración estándar
                dir_gpio,         // Dirección calculada
                pasos_a_mover     // Pasos calculados
            );
        }
    } 
}

// --- API DE CONTROL (HUECOS PARA LA WEB) ---
// (Sin cambios)
void Exobrazo::moverMotorWeb(int motorIndex, float aceleracion, uint32_t direccion, int pasos) {
    this->modoActual = ModoControl::WEB;
    if (motorIndex < 0 || motorIndex > 2) return;
    motores[motorIndex]->activarMovimiento(aceleracion, direccion, pasos);
}

void Exobrazo::setModoPrueba() {
    this->modoActual = ModoControl::PRUEBA;
}