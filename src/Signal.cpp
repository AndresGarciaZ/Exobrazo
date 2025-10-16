#include "Signal.h"
#include <driver/gpio.h>

Signal::Signal(gpio_num_t pin, float aceleracion, int pasos) 
    : pin(pin), aceleracion(aceleracion), pasos(pasos) {
    
    this->estado = false;
    this->direccion = 1;  // HIGH por defecto
    
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_pulldown_en(pin);
}

bool Signal::obtenerEstado() {
    // Siempre leer el estado físico del pin
    estado = gpio_get_level(pin);
    return estado;
}