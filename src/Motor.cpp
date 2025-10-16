#include "Motor.h"
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_timer.h>
#include <math.h>
#include <esp_rom_sys.h>

Motor::Motor(gpio_num_t step_pin, gpio_num_t dir_pin,
             adc1_channel_t adc_channel, Signal& signal,
             float accel_max, float vel_max, int pasos_por_rev,
             float limite_min, float limite_max)
    : step_pin(step_pin), dir_pin(dir_pin), 
      adc_channel(adc_channel), signal(signal),
      accel_max(accel_max), vel_max(vel_max), pasos_por_rev(pasos_por_rev),
      limite_min(limite_min), limite_max(limite_max),
      posicion_actual(0.0) {
    
    gpio_set_direction(step_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(dir_pin, GPIO_MODE_OUTPUT);
    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(adc_channel, ADC_ATTEN_DB_11);
}

void Motor::activarMovimiento(float aceleracion, uint32_t direccion, int pasos) {
    // 1. Validación de parámetros
    if (pasos <= 0) return;
    
    // 2. Limitar aceleración a la máxima del motor
    if (aceleracion > accel_max) {
        aceleracion = accel_max;
    }
    
    // 3. Configurar dirección
    gpio_set_level(dir_pin, direccion);
    
    // 4. Calcular velocidad máxima usable (limitada por el motor y la aceleración)
    float vel_max_usable = calcularVelocidadMaxima(aceleracion, pasos);
    
    // 5. Calcular pasos para cada fase del perfil trapezoidal
    int pasos_aceleracion = calcularPasosAceleracion(aceleracion, vel_max_usable);
    int pasos_desaceleracion = pasos_aceleracion;
    int pasos_vel_constante = pasos - pasos_aceleracion - pasos_desaceleracion;
    
    // 6. Ajustar si no hay suficiente distancia para velocidad constante
    if (pasos_vel_constante < 0) {
        pasos_aceleracion = pasos / 2;
        pasos_desaceleracion = pasos - pasos_aceleracion;
        pasos_vel_constante = 0;
    }
    
    // 7. Calcular delays basados en la velocidad
    int delay_min = (int)(1000000 / (2 * vel_max_usable)); // Delay para velocidad máxima
    int delay_max = delay_min * 3; // Delay inicial más lento
    
    // 8. Aplicar límites de seguridad a los delays
    if (delay_min < 100) delay_min = 100;   // Máximo ~5000 pasos/segundo
    if (delay_max > 5000) delay_max = 5000; // Mínimo ~100 pasos/segundo
    
    // 9. Ejecutar las tres fases del movimiento
    
    // Fase 1: Aceleración (disminuir delay gradualmente)
    for (int i = 0; i < pasos_aceleracion; i++) {
        int delay_actual = delay_max - (delay_max - delay_min) * i / pasos_aceleracion;
        ejecutarPaso(delay_actual);
    }
    
    // Fase 2: Velocidad constante
    for (int i = 0; i < pasos_vel_constante; i++) {
        ejecutarPaso(delay_min);
    }
    
    // Fase 3: Desaceleración (aumentar delay gradualmente)
    for (int i = 0; i < pasos_desaceleracion; i++) {
        int delay_actual = delay_min + (delay_max - delay_min) * i / pasos_desaceleracion;
        ejecutarPaso(delay_actual);
    }
    
    // 10. Actualizar posición actual
    posicion_actual = leerPosicion();
}

float Motor::calcularVelocidadMaxima(float aceleracion, int pasos) {
    // Calcular velocidad máxima alcanzable con la aceleración dada
    float vel_max_aceleracion = sqrt(2 * aceleracion * pasos);
    
    // Convertir velocidad máxima del motor (rad/s) a pasos/segundo
    float vel_max_motor_pasos = vel_max * (pasos_por_rev / (2 * M_PI));
    
    // Usar la menor de las dos velocidades
    float vel_max_usable = (vel_max_aceleracion < vel_max_motor_pasos) ? 
                           vel_max_aceleracion : vel_max_motor_pasos;
    
    return vel_max_usable;
}

int Motor::calcularPasosAceleracion(float aceleracion, float vel_max_usable) {
    // Calcular pasos necesarios para alcanzar la velocidad máxima
    // usando la fórmula: v² = 2 * a * s
    return (int)((vel_max_usable * vel_max_usable) / (2 * aceleracion));
}

float Motor::leerPosicion() {
    int adc_val = adc1_get_raw(adc_channel);
    // Convertir valor ADC a grados (0-300° típico para potenciómetros)
    float posicion_grados = (adc_val / 4095.0) * 300.0;
    return posicion_grados;
}

void Motor::ejecutarPaso(int delay_us) {
    gpio_set_level(step_pin, 1);
    esp_rom_delay_us(delay_us);
    gpio_set_level(step_pin, 0);
    esp_rom_delay_us(delay_us);
}