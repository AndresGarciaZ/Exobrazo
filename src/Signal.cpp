#include "Signal.h"
#include <driver/gpio.h>

// Constructor actualizado para recibir 'velocidad'
Signal::Signal(gpio_num_t pin, float velocidad, int pasos) 
    : pin(pin), velocidad(velocidad), pasos(pasos) {
    
    this->estado_logico = false; 
    this->direccion = 1; 
    
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    // gpio_pulldown_en(pin); // Comentado porque tienes resistencias físicas externas (R1-R4)
}

bool Signal::obtenerEstado() {
    // Lectura física
    bool estado_fisico = gpio_get_level(pin);
    
    // Lógica OR: Físico O Virtual
    return estado_fisico || estado_logico;
}

void Signal::establecerEstado(bool nuevo_estado) {
    this->estado_logico = nuevo_estado;
}