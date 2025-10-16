#ifndef MOTOR_H
#define MOTOR_H

#include <driver/gpio.h>
#include <driver/adc.h>
#include "Signal.h"

class Motor {
private:
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    adc1_channel_t adc_channel;
    Signal& signal;

    float accel_max;
    float vel_max;
    int pasos_por_rev;  // Pasos necesarios para una revolución completa
    float posicion_actual;
    float limite_min;
    float limite_max;

public:
    Motor(gpio_num_t step_pin, gpio_num_t dir_pin,
          adc1_channel_t adc_channel, Signal& signal,
          float accel_max, float vel_max, int pasos_por_rev,
          float limite_min, float limite_max);

    void activarMovimiento(float aceleracion, uint32_t direccion, int pasos);
    float leerPosicion();

    float getLimiteMin() const { return limite_min; }
    float getLimiteMax() const { return limite_max; }
    float getPosicionActual() const { return posicion_actual; }
    int getPasosPorRev() const { return pasos_por_rev; }

private:
    void ejecutarPaso(int delay_us);
    float calcularVelocidadMaxima(float aceleracion, int pasos);
    int calcularPasosAceleracion(float aceleracion, float vel_max_usable);
};

#endif // MOTOR_H