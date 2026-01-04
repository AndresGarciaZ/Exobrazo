#include "Motor.h"
#include <esp_timer.h>
#include <esp_rom_sys.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Motor::Motor(gpio_num_t step_pin, gpio_num_t dir_pin, gpio_num_t enable_pin,
             Signal& signal, float accel_max, float vel_max, int pasos_por_rev,
             float limite_min, float limite_max)
    : step_pin(step_pin), dir_pin(dir_pin), enable_pin(enable_pin), signal(signal),
      accel_max(accel_max), vel_max_abs(vel_max), pasos_por_rev(pasos_por_rev),
      limite_min(limite_min), limite_max(limite_max),
      posicion_actual(0.0f), en_movimiento(false), velocidad_actual(0.0f), 
      velocidad_objetivo(0.0f), pasos_totales(0), paso_actual(0), 
      pasos_acel(0), paso_decel(0), delta_v_paso(0.0f),
      direccion_actual(0), tiempo_ultimo_paso(0), delay_actual(0) 
{
    gpio_set_direction(step_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(dir_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(enable_pin, GPIO_MODE_OUTPUT);
    deshabilitar();
}

// ... (El resto de funciones activarMovimiento, actualizar, etc. se mantienen igual que arriba) ...

void Motor::activarMovimiento(uint32_t direccion, int pasos, float velocidad_custom) {
    if (pasos <= 0) return;
    const float alpha = (2.0f * M_PI) / (float)pasos_por_rev;
    float v_target = (velocidad_custom > 0) ? velocidad_custom : vel_max_abs;

    if (en_movimiento && direccion == direccion_actual) {
        pasos_totales += pasos;
        int32_t dist_frenado = (int32_t)((velocidad_actual * velocidad_actual) / (2.0f * accel_max * alpha));
        paso_decel = pasos_totales - dist_frenado;
        return;
    }

    habilitar();
    direccion_actual = direccion;
    gpio_set_level(dir_pin, direccion);

    int32_t n_acel = (int32_t)((v_target * v_target) / (2.0f * accel_max * alpha));
    if (n_acel > pasos / 2) { 
        pasos_acel = pasos / 2;
        paso_decel = pasos - pasos_acel;
    } else { 
        pasos_acel = n_acel;
        paso_decel = pasos - n_acel;
    }

    delta_v_paso = (accel_max * alpha) / fmaxf(v_target * 0.5f, 1.0f);
    velocidad_actual = sqrtf(2.0f * accel_max * alpha); 
    if (velocidad_actual < 1.0f) velocidad_actual = 1.0f;

    pasos_totales = pasos;
    paso_actual = 0;
    velocidad_objetivo = v_target;
    calcularNuevoDelay();
    tiempo_ultimo_paso = esp_timer_get_time();
    en_movimiento = true;
}

bool Motor::actualizar() {
    if (!en_movimiento) return false;
    int64_t ahora = esp_timer_get_time();
    if (ahora >= tiempo_ultimo_paso + delay_actual) {
        gpio_set_level(step_pin, 1);
        esp_rom_delay_us(10); 
        gpio_set_level(step_pin, 0);
        tiempo_ultimo_paso += delay_actual; 
        paso_actual++;
        if (paso_actual >= pasos_totales) { detener(); return false; }
        if (paso_actual < pasos_acel) {
            velocidad_actual += delta_v_paso;
            if (velocidad_actual > velocidad_objetivo) velocidad_actual = velocidad_objetivo;
        } 
        else if (paso_actual > paso_decel) {
            velocidad_actual -= delta_v_paso;
            if (velocidad_actual < 1.0f) velocidad_actual = 1.0f;
        }
        calcularNuevoDelay();
    }
    return true;
}

void Motor::calcularNuevoDelay() {
    const float alpha = (2.0f * M_PI) / (float)pasos_por_rev;
    delay_actual = (int32_t)((alpha / fmaxf(velocidad_actual, 0.1f)) * 1000000.0f);
}

void Motor::detener() { en_movimiento = false; deshabilitar(); }
void Motor::habilitar() { gpio_set_level(enable_pin, 0); }
void Motor::deshabilitar() { gpio_set_level(enable_pin, 1); }

/* * En Exobrazo.cpp (o Motor.cpp)
 * Función para leer y convertir el valor del ADC a grados.
 */
float Motor::leerPosicion(adc1_channel_t channel) {
    // 1. Limpieza del canal ADC (Lectura fantasma)
    // Esto soluciona por qué el valor no se iba al extremo al desconectar
    adc1_get_raw(channel); 
    
    // 2. Tu código original tal cual funcionaba
    int adc_val = adc1_get_raw(channel);
    float posicion_grados = (adc_val / 4095.0f) * 270.0f - 135.0f;
    
    this->posicion_actual = posicion_grados;
    return this->posicion_actual;
}