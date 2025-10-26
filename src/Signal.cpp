#include "Signal.h"
#include <driver/gpio.h>

Signal::Signal(gpio_num_t pin, float aceleracion, int pasos) 
    : pin(pin), aceleracion(aceleracion), pasos(pasos) {
    
    // El estado lógico/forzado empieza en false por defecto.
    this->estado_logico = false; 
    
    this->direccion = 1;  // HIGH por defecto
    
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_pulldown_en(pin);
}

/**
 * @brief Obtiene el estado de la señal (LÓGICA "OR" COMBINADA)
 */
bool Signal::obtenerEstado() {
    // 1. Lee el estado físico del pin (botón individual)
    bool estado_fisico = gpio_get_level(pin);
    
    // 2. La señal se considera activa si:
    //    - Su pin físico está presionado (estado_fisico == true)
    //    - O si la señal general ha forzado su estado (estado_logico == true)
    return estado_fisico || estado_logico;
}

/**
 * @brief Establece el estado lógico (usado por GSignal)
 */
void Signal::establecerEstado(bool nuevo_estado) {
    // Este método ahora SÍ tiene un propósito claro:
    // GSignal lo usa para forzar el estado.
    this->estado_logico = nuevo_estado;
}