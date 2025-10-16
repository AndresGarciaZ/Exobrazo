#include "GSignal.h"
#include <driver/gpio.h>

GSignal::GSignal(gpio_num_t pin) : pin(pin), estado(false), estado_anterior(false), 
                                   numSignals(0), gsignal_activo(false) {
    for (int i = 0; i < 3; i++) {
        signals[i] = nullptr;
    }
    
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_pulldown_en(pin);
}

void GSignal::agregarSignal(Signal* signal) {
    if (numSignals < 3 && signal != nullptr) {
        signals[numSignals] = signal;
        numSignals++;
    }
}

bool GSignal::obtenerEstado() {
    return estado;
}

void GSignal::actualizarEstado() {
    estado_anterior = estado;
    estado = gpio_get_level(pin);
    
    if (haCambiadoAActivo()) {
        gsignal_activo = true;
        forzarActivacionTodas();
    }
    else if (haCambiadoAInactivo()) {
        gsignal_activo = false;
        liberarTodas();
    }
}

bool GSignal::haCambiadoAActivo() {
    return !estado_anterior && estado;
}

bool GSignal::haCambiadoAInactivo() {
    return estado_anterior && !estado;
}

void GSignal::forzarActivacionTodas() {
    // Cuando GSignal se activa, establecemos el estado de todas las señales a true
    for (int i = 0; i < numSignals; i++) {
        if (signals[i] != nullptr) {
            signals[i]->establecerEstado(true);  // Forzar estado a true (activo)
        }
    }
    
}

void GSignal::liberarTodas() {
    // Cuando GSignal se desactiva, establecemos el estado de todas las señales a false
    for (int i = 0; i < numSignals; i++) {
        if (signals[i] != nullptr) {
            signals[i]->establecerEstado(false);  // Forzar estado a false (inactivo)
        }
    }
    
}