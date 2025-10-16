#ifndef SIGNAL_H
#define SIGNAL_H

#include <driver/gpio.h>

class Signal {
private:
    gpio_num_t pin;
    bool estado;
    uint32_t direccion;
    float aceleracion;
    int pasos;

public:
    Signal(gpio_num_t pin, float aceleracion, int pasos);
    bool obtenerEstado();
    uint32_t obtenerDireccion() const { return direccion; }
    void establecerDireccion(uint32_t nueva_direccion) { direccion = nueva_direccion; }
    void establecerEstado(bool nuevo_estado) { estado = nuevo_estado; }
    float getAceleracion() const { return aceleracion; }
    int getPasos() const { return pasos; }
};

#endif // SIGNAL_H