#include "Exobrazo.h"
#include "defines.h"

Exobrazo::Exobrazo() :
    // 1. Inicializar Señales (pin, acel_defecto, pasos_defecto)
    signalMotor1(SIGNAL_PIN_1, ACCEL_PRUEBA_STD, PASOS_PRUEBA_STD),
    signalMotor2(SIGNAL_PIN_2, ACCEL_PRUEBA_STD, PASOS_PRUEBA_STD),
    signalMotor3(SIGNAL_PIN_3, ACCEL_PRUEBA_STD, PASOS_PRUEBA_STD),

    // 2. Inicializar Motores (step, dir, adc, signal_ref, pasos_rev, lim_min, lim_max, accel_max, vel_max)
    motor1(STEP_PIN_1, DIR_PIN_1, ADC_PIN_1, signalMotor1, 
           PASOS_REV_M1, LIM_MIN_M1, LIM_MAX_M1, ACCEL_MAX_M1, VEL_MAX_M1),
           
    motor2(STEP_PIN_2, DIR_PIN_2, ADC_PIN_2, signalMotor2, 
           PASOS_REV_M2, LIM_MIN_M2, LIM_MAX_M2, ACCEL_MAX_M2, VEL_MAX_M2),
           
    motor3(STEP_PIN_3, DIR_PIN_3, ADC_PIN_3, signalMotor3, 
           PASOS_REV_M3, LIM_MIN_M3, LIM_MAX_M3, ACCEL_MAX_M3, VEL_MAX_M3),

    // 3. Inicializar Señal General y Monitor
    senalGeneral(GSIGNAL_PIN),
    monitor()
{
    // Llenar el arreglo público de punteros a motores
    motores[0] = &motor1;
    motores[1] = &motor2;
    motores[2] = &motor3;

    // Inicializar los estados de la aplicación
    modoActual = ModoControl::PRUEBA; // Arranca en modo botones
    dirEstadosPrueba[0] = 1;
    dirEstadosPrueba[1] = 1;
    dirEstadosPrueba[2] = 1;
}

void Exobrazo::iniciar() {
    // Vincular las 3 señales de los motores al control del botón general
    senalGeneral.agregarSignal(&signalMotor1);
    senalGeneral.agregarSignal(&signalMotor2);
    senalGeneral.agregarSignal(&signalMotor3);
}

/**
 * @brief Bucle de lógica principal (Núcleo 0)
 */
void Exobrazo::ejecutarCicloPrincipal() {
    
    // 1. SI ESTAMOS EN MODO WEB, NO ESCUCHAR BOTONES.
    if (modoActual == ModoControl::WEB) {
        return; 
    }

    // 2. (MODO PRUEBA) Actualizar el estado del botón general
    senalGeneral.actualizarEstado();

    // 3. (MODO PRUEBA) Iterar sobre cada motor
    for (int i = 0; i < 3; i++) {

        // 4. Comprobar si la señal de este motor está activa
        if (motores[i]->signal.obtenerEstado()) {

            // 5. LÓGICA DE REBOTE (El cerebro)
            
            // --- ESTA ES LA CORRECCIÓN ---
            // Llamamos al método público leerPosicion() para obtener el ángulo real
            float posActual = motores[i]->leerPosicion(); 
            // -----------------------------

            float limMin = motores[i]->getLimiteMin();
            float limMax = motores[i]->getLimiteMax();

            // Comprobar si los límites están invertidos (Motor 3)
            if (limMin > limMax) { // Lógica M3 (ej: 0 a -100)
                if (posActual >= limMin) { // En o sobre 0°
                    dirEstadosPrueba[i] = -1; // Mover hacia -100°
                } else if (posActual <= limMax) { // En o bajo -100°
                    dirEstadosPrueba[i] = 1;  // Mover hacia 0°
                }
            } else { // Lógica normal M1, M2 (ej: -40 a 130)
                if (posActual >= limMax) {
                    dirEstadosPrueba[i] = -1; // Mover hacia Mínimo
                } else if (posActual <= limMin) {
                    dirEstadosPrueba[i] = 1;  // Mover hacia Máximo
                }
            }

            // 6. DAR LA ORDEN
            uint32_t dir_gpio = (dirEstadosPrueba[i] == 1) ? 1 : 0; 
            
            motores[i]->activarMovimiento(
                ACCEL_PRUEBA_STD, 
                dir_gpio, 
                PASOS_PRUEBA_STD
            );
            // El programa se bloquea aquí hasta que el motor termine.
        }
    } // fin del for
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