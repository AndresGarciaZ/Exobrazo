#ifndef GSIGNAL_H
#define GSIGNAL_H

#include <driver/gpio.h>
#include "Signal.h"

class GSignal {
private:
    gpio_num_t pin;
    bool estado;
    bool estado_anterior;
    Signal* signals[3];
    int numSignals;
    bool gsignal_activo;

public:
    GSignal(gpio_num_t pin);
    void agregarSignal(Signal* signal);
    bool obtenerEstado();
    void actualizarEstado();
    bool estaActivo() const { return gsignal_activo; }

private:
    void forzarActivacionTodas();
    void liberarTodas();
    bool haCambiadoAActivo();
    bool haCambiadoAInactivo();
};

#endif // GSIGNAL_H