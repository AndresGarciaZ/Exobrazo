#include "Exobrazo.h"
#include <esp_timer.h> 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Exobrazo::Exobrazo() :
    monitor(),
    signalMotor1(SIGNAL_PIN_1, VEL_PRUEBA_STD, 0), 
    signalMotor2(SIGNAL_PIN_2, VEL_PRUEBA_STD, 0),
    signalMotor3(SIGNAL_PIN_3, VEL_PRUEBA_STD, 0),
    motor1(STEP_PIN_1, DIR_PIN_1, ENABLE_PIN_1, signalMotor1, ACCEL_MAX_M1, VEL_MAX_M1, PASOS_REV_M1, LIM_MIN_M1, LIM_MAX_M1),
    motor2(STEP_PIN_2, DIR_PIN_2, ENABLE_PIN_2, signalMotor2, ACCEL_MAX_M2, VEL_MAX_M2, PASOS_REV_M2, LIM_MIN_M2, LIM_MAX_M2),
    motor3(STEP_PIN_3, DIR_PIN_3, ENABLE_PIN_3, signalMotor3, ACCEL_MAX_M3, VEL_MAX_M3, PASOS_REV_M3, LIM_MIN_M3, LIM_MAX_M3),
    senalGeneral(GSIGNAL_PIN),
    modoActual(ModoControl::PRUEBA)
{
    motores[0] = &motor1; motores[1] = &motor2; motores[2] = &motor3;
    // Inicializamos direcciones en 1 (Horario/Positivo)
    dirEstadosPrueba[0] = 1; dirEstadosPrueba[1] = 1; dirEstadosPrueba[2] = 1;
    adc_pins[0] = (adc1_channel_t)ADC_PIN_1;
    adc_pins[1] = (adc1_channel_t)ADC_PIN_2;
    adc_pins[2] = (adc1_channel_t)ADC_PIN_3;
}

void Exobrazo::iniciar() {
    senalGeneral.actualizarEstado();
    for(int i = 0; i < 3; i++) {
        motores[i]->detener();
    }
}

void Exobrazo::ejecutarCicloPrincipal() {
    // 1. SINCRONIZACIÓN Y LECTURA (Núcleo 0)
    static int64_t ultima_lectura = 0;
    int64_t ahora = esp_timer_get_time();
    if ((ahora - ultima_lectura) < 20000) return; // 20ms
    ultima_lectura = ahora;

    // Actualizar ADC de todos los motores
    for (int i = 0; i < 3; i++) { motores[i]->leerPosicion(adc_pins[i]); }

    // Actualizar estado del botón general y botones individuales
    senalGeneral.actualizarEstado(); 
    bool estadoGeneral = senalGeneral.obtenerEstado();
    
    bool estadosBotones[3];
    for (int i = 0; i < 3; i++) { 
        estadosBotones[i] = motores[i]->signal.obtenerEstado(); 
    }
    
    // Enviar datos al monitor serial (Tarea pesada, Core 0)
    monitor.actualizarBotones(estadosBotones);
    monitor.actualizarDirecciones(dirEstadosPrueba);
    monitor.actualizarDatos(motores);

    if (modoActual == ModoControl::WEB) return;

    // 2. LÓGICA DE REBOTE Y MOVIMIENTO (Núcleo 0)
    for (int i = 0; i < 3; i++) {
        // Se activa si su botón individual está presionado O si el botón general lo está
        bool botonPresionado = (estadosBotones[i] || estadoGeneral);

        if (botonPresionado && !motores[i]->estaOcupado()) {
            
            float pos = motores[i]->getPosicionActual();
            float lMin = motores[i]->getLimiteMin();
            float lMax = motores[i]->getLimiteMax();
            
            /* * LÓGICA DE REBOTE DETERMINISTA:
             * dirEstadosPrueba: 1 = Horario (Hacia Positivo), -1 = Antihorario (Hacia Negativo)
             */
            if (dirEstadosPrueba[i] == 1) { // Intentando ir a Positivo
                if (pos >= lMax) {
                    dirEstadosPrueba[i] = -1; // Rebota a Negativo
                }
            } else { // Intentando ir a Negativo
                if (pos <= lMin) {
                    dirEstadosPrueba[i] = 1; // Rebota a Positivo
                }
            }

            // CÁLCULO DE PASOS PARA 20 GRADOS
            // (ANGULO_PRUEBA_STD / 360) * PASOS_REV
            int p_rev = motores[i]->getPasosPorRev();
            int pasos_20 = (int)((ANGULO_PRUEBA_STD * p_rev) / 360.0f);
            
            float vel = motores[i]->signal.getVelocidad();
            
            // Mapeo de dirección interna a señal GPIO:
            // dirEstadosPrueba 1 (Horario) -> dir_gpio 1
            // dirEstadosPrueba -1 (Antihorario) -> dir_gpio 0
            uint32_t dir_gpio = (dirEstadosPrueba[i] == 1) ? 1 : 0; 
            
            // Ordenar al motor que inicie el bloque de pasos
            motores[i]->activarMovimiento(dir_gpio, pasos_20, vel);
        }
    } 
}

void Exobrazo::moverMotorWeb(int motorIndex, float vel_custom, uint32_t direccion, int pasos) {
    if (motorIndex < 0 || motorIndex > 2) return;

    float posActual = motores[motorIndex]->getPosicionActual();
    
    // Redundancia de seguridad: comprobar límites antes de activar GPIO
    if (direccion == 1 && posActual >= motores[motorIndex]->getLimiteMax()) return;
    if (direccion == 0 && posActual <= motores[motorIndex]->getLimiteMin()) return;

    // Ejecutar con la velocidad recibida desde la web
    motores[motorIndex]->activarMovimiento(direccion, pasos, vel_custom);
}

void Exobrazo::setModoPrueba() { 
    this->modoActual = ModoControl::PRUEBA; 
    for(int i=0; i<3; i++) motores[i]->detener(); 
}

void Exobrazo::setModoWeb() { 
    this->modoActual = ModoControl::WEB; 
    for(int i=0; i<3; i++) motores[i]->detener(); 
}

Exobrazo::ModoControl Exobrazo::getModoActual() const { 
    return this->modoActual; 
}