#ifndef SIGNAL_H
#define SIGNAL_H

#include <driver/gpio.h>

class Signal {
private:
    gpio_num_t pin;
    
    /**
     * @brief Almacena el estado "lógico" o "forzado".
     * Este estado es el que GSignal puede modificar (true/false).
     */
    bool estado_logico; 
    
    uint32_t direccion;
    float velocidad;
    int pasos;

public:
    Signal(gpio_num_t pin, float aceleracion, int pasos);

    /**
     * @brief Comprueba si la señal está activa.
     * @return true si el pin físico está HIGH O si el estado_logico es true.
     */
    bool obtenerEstado();
    
    uint32_t obtenerDireccion() const { return direccion; }
    void establecerDireccion(uint32_t nueva_direccion) { direccion = nueva_direccion; }
    
    /**
     * @brief Establece el estado lógico/forzado de la señal.
     * Usado por GSignal para la activación/desactivación general.
     */
    void establecerEstado(bool nuevo_estado);
    
    float getVelocidad() const { return velocidad; }
    void setVelocidad(float nueva_velocidad) { velocidad = nueva_velocidad; }
};

#endif // SIGNAL_H