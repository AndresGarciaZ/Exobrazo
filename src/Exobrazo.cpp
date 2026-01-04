#include "Exobrazo.h"
#include <esp_timer.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

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

void Exobrazo::moverMotorWeb(int motorIndex, float vel_custom, uint32_t direccion, float angulo_grados) {
    if (motorIndex < 0 || motorIndex > 2) return;

    // 1. FÓRMULA DE CONVERSIÓN: Grados a Pasos
    // pasos = (ángulo_deseado / 360) * pasos_totales_por_rev
    float pasos_por_rev = (float)motores[motorIndex]->getPasosPorRev();
    int pasos_calculados = (int)((angulo_grados / 360.0f) * pasos_por_rev);

    // Evitar procesar si el cálculo resulta en 0 pasos
    if (pasos_calculados <= 0) return;

    // 2. Validación de seguridad con la posición actual del ADC
    float posActual = motores[motorIndex]->getPosicionActual();
    
    // Bloqueo si intenta superar los límites establecidos
    if (direccion == 1 && posActual >= motores[motorIndex]->getLimiteMax()) return;
    if (direccion == 0 && posActual <= motores[motorIndex]->getLimiteMin()) return;

    // 3. Ejecutar movimiento físico
    motores[motorIndex]->activarMovimiento(direccion, pasos_calculados, vel_custom);
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

void Exobrazo::ejecutarMovimientoTerapeutico() {
    // 1. Constantes de Control
    const float VEL_M1 = 1.0f;
    const float VEL_M2 = 0.8f;
    const float VEL_M3 = 0.8f;
    const float TOLERANCIA = 1.5f; // Margen de error permitido en grados
    const int MAX_CORRECCIONES = 3; // Intentos para ajustar la posición si falla

    // 2. Factores de Corrección (Offsets)
    const float offsets[3] = {0.0f, 0.0f, 0.0f};
    const float vels[3] = {VEL_M1, VEL_M2, VEL_M3};

    /**
     * Helper de Lazo Cerrado: Mueve, verifica y corrige si es necesario.
     */
    auto moverEjeValidado = [&](int id, float anguloObjetivo) {
        int reintentos = 0;
        bool enDestino = false;

        while (!enDestino && reintentos < MAX_CORRECCIONES) {
            // Sincronizar ADC para tener la lectura más reciente
            motores[id]->leerPosicion(adc_pins[id]);
            
            float posReal = motores[id]->getPosicionActual() + offsets[id];
            float delta = anguloObjetivo - posReal;

            // VALIDACIÓN: ¿Estamos en el lugar correcto?
            if (fabs(delta) <= TOLERANCIA) {
                enDestino = true;
                break;
            }

            // Si no estamos en destino, calculamos dirección y pasos necesarios
            uint32_t dir = (delta > 0) ? 1 : 0;
            int pasos = (int)((fabs(delta) / 360.0f) * motores[id]->getPasosPorRev());

            // Ejecutar movimiento solo si es seguro
            if (anguloObjetivo >= motores[id]->getLimiteMin() && 
                anguloObjetivo <= motores[id]->getLimiteMax()) {
                
                motores[id]->activarMovimiento(dir, pasos, vels[id]);

                // Esperar a que el motor termine físicamente
                while (motores[id]->estaOcupado()) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
            reintentos++;
            vTaskDelay(pdMS_TO_TICKS(50)); // Pausa breve para estabilización del sensor
        }
    };

    // --- INICIO DE LA SECUENCIA TERAPÉUTICA VALIDADA ---

    // Paso 0: Posicionamiento Inicial con validación
    moverEjeValidado(0, 0.0f);   // Base a 0°
    moverEjeValidado(1, -90.0f); // Hombro a -90°
    moverEjeValidado(2, 0.0f);   // Codo a 0°

    // Paso 1: Rutina de Motor 2 (Hombro)
    moverEjeValidado(1, 0.0f);
    vTaskDelay(pdMS_TO_TICKS(1000));

    moverEjeValidado(1, 90.0f);
    vTaskDelay(pdMS_TO_TICKS(1000));

    moverEjeValidado(1, 0.0f);

    // Paso 2: Rutina de Motor 3 (Codo)
    moverEjeValidado(2, 90.0f);
    vTaskDelay(pdMS_TO_TICKS(1000));

    moverEjeValidado(2, 0.0f);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Paso 3: Finalización
    moverEjeValidado(1, -90.0f);
}