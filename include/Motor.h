#ifndef MOTOR_H
#define MOTOR_H

#include <driver/gpio.h>
#include <driver/adc.h>
#include "Signal.h" 
#include <math.h>

class Motor {
private:
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    gpio_num_t enable_pin;
    
public:
    Signal& signal;

private:
    float accel_max;
    float vel_max_abs;
    int pasos_por_rev;
    float limite_min;
    float limite_max;
    float posicion_actual;
    bool en_movimiento;
    float velocidad_actual;
    float velocidad_objetivo;
    int32_t pasos_totales;
    int32_t paso_actual;
    int32_t pasos_acel;
    int32_t paso_decel;
    float delta_v_paso;
    uint32_t direccion_actual;
    int64_t tiempo_ultimo_paso; 
    int32_t delay_actual; 

public:
    Motor(gpio_num_t step_pin, gpio_num_t dir_pin, gpio_num_t enable_pin,
          Signal& signal, float accel_max, float vel_max, int pasos_por_rev,
          float limite_min, float limite_max);

    void activarMovimiento(uint32_t direccion, int pasos, float velocidad_custom = -1.0f);
    bool actualizar();
    void detener();
    float leerPosicion(adc1_channel_t channel);
    
    bool estaOcupado() const { return en_movimiento; }
    float getPosicionActual() const { return posicion_actual; }
    float getLimiteMin() const { return limite_min; }
    float getLimiteMax() const { return limite_max; }
    int   getPasosPorRev() const { return pasos_por_rev; }

private:
    void calcularNuevoDelay();
    void habilitar();
    void deshabilitar();
};

#endif