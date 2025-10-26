#include "GSignal.h"
#include <driver/gpio.h>

/**
 * @brief Constructor de GSignal
 */
GSignal::GSignal(gpio_num_t pin) 
    : pin(pin),                 // Asigna el pin
      estado(false),            // Estado actual inicia en falso
      estado_anterior(false),   // Estado anterior inicia en falso
      numSignals(0),            // Aún no hay señales registradas
      gsignal_activo(false) {   // El 'latch' general inicia apagado
    
    // Inicializar el array de punteros a nullptr
    for (int i = 0; i < 3; i++) {
        signals[i] = nullptr;
    }
    
    // Configurar el pin físico
    gpio_set_direction(pin, GPIO_MODE_INPUT);  // Como entrada
    gpio_pulldown_en(pin); // Con resistencia pull-down (lee 0 por defecto)
}

/**
 * @brief Añade un puntero a un objeto Signal al array 'signals'
 */
void GSignal::agregarSignal(Signal* signal) {
    // Asegurarse de no desbordar el array y que el puntero es válido
    if (numSignals < 3 && signal != nullptr) {
        signals[numSignals] = signal; // Almacena el puntero
        numSignals++;                 // Incrementa el contador
    }
}

/**
 * @brief Devuelve el estado del pin leído en el último 'actualizarEstado()'
 */
bool GSignal::obtenerEstado() {
    return estado;
}

/**
 * @brief Método principal de sondeo (polling), debe llamarse en bucle.
 */
void GSignal::actualizarEstado() {
    // 1. Guardar el estado del ciclo anterior
    estado_anterior = estado;
    
    // 2. Leer el estado físico actual del pin
    estado = gpio_get_level(pin);
    
    // 3. Comprobar si ha ocurrido un flanco de subida (pulsación)
    if (haCambiadoAActivo()) {
        gsignal_activo = true;      // Activa el 'latch'
        forzarActivacionTodas();    // Forza todas las señales a 'true'
    }
    // 4. Comprobar si ha ocurrido un flanco de bajada (liberación)
    else if (haCambiadoAInactivo()) {
        gsignal_activo = false;     // Desactiva el 'latch'
        liberarTodas();             // Libera todas las señales a 'false'
    }
}

/**
 * @brief Detección de flanco de subida
 */
bool GSignal::haCambiadoAActivo() {
    // Verdadero solo si ANTES estaba en 0 (false) y AHORA está en 1 (true)
    return !estado_anterior && estado;
}

/**
 * @brief Detección de flanco de bajada
 */
bool GSignal::haCambiadoAInactivo() {
    // Verdadero solo si ANTES estaba en 1 (true) y AHORA está en 0 (false)
    return estado_anterior && !estado;
}

/**
 * @brief Itera sobre las señales registradas y fuerza su estado lógico a 'true'
 */
void GSignal::forzarActivacionTodas() {
    // Recorre todas las señales que se hayan agregado
    for (int i = 0; i < numSignals; i++) {
        if (signals[i] != nullptr) {
            // Llama al método de Signal que cambia su 'estado_logico'
            signals[i]->establecerEstado(true); 
        }
    }
}

/**
 * @brief Itera sobre las señales registradas y fuerza su estado lógico a 'false'
 */
void GSignal::liberarTodas() {
    // Recorre todas las señales que se hayan agregado
    for (int i = 0; i < numSignals; i++) {
        if (signals[i] != nullptr) {
            // Llama al método de Signal que cambia su 'estado_logico'
            signals[i]->establecerEstado(false);
        }
    }
}