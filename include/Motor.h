#ifndef MOTOR_H
#define MOTOR_H

#include <driver/gpio.h>
#include <driver/adc.h>
#include "Signal.h" 

class Motor {

private:
    // --- 1. Variables privadas (en orden) ---
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    adc1_channel_t adc_channel_RESERVED; 

public:
    // --- 2. Variables públicas (en orden) ---
    /**
     * @brief Referencia pública a la señal del motor.
     */
    Signal& signal; // <--- Movida aquí (4ta en la lista)

private:
    // --- 3. Resto de variables privadas (en orden) ---
    float accel_max;
    float vel_max;
    int   pasos_por_rev;
    float limite_min;
    float limite_max;
    float posicion_actual;

// --- Fin del orden corregido ---

public:
    /**
     * @brief Constructor de la clase Motor.
     */
 Motor(gpio_num_t step_pin, gpio_num_t dir_pin,
          Signal& signal,
          float accel_max, float vel_max, int pasos_por_rev,
          float limite_min, float limite_max);

    void activarMovimiento(float aceleracion, uint32_t direccion, int pasos);
    float leerPosicion(adc1_channel_t channel);

    // --- Getters 
    float getLimiteMin() const { return limite_min; }
    float getLimiteMax() const { return limite_max; }
    float getPosicionActual() const { return posicion_actual; }
    int getPasosPorRev() const { return pasos_por_rev; }

private:
    // --- Funciones privadas ---
    void ejecutarPaso(int delay_us);
    float calcularVelocidadMaxima(float aceleracion, float despl_angular_total);
    float calcularDesplazamientoAceleracion(float aceleracion, float vel_max_usable);
};

#endif // MOTOR_H