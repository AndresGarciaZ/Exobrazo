#include "Motor.h"
#include <math.h>         // Para M_PI, sqrt() y fminf()
#include <esp_timer.h>    // Para esp_rom_delay_us()
#include <esp_rom_sys.h>  // (Incluido en el original, esp_rom_delay_us puede estar aquí)



// --- Constructor ---
Motor::Motor(gpio_num_t step_pin, gpio_num_t dir_pin, gpio_num_t enable_pin,
             Signal& signal,
             float accel_max, float vel_max, int pasos_por_rev,
             float limite_min, float limite_max)
    : step_pin(step_pin), dir_pin(dir_pin), enable_pin(enable_pin),
      signal(signal),
      accel_max(accel_max), vel_max(vel_max), pasos_por_rev(pasos_por_rev),
      limite_min(limite_min), limite_max(limite_max),
      posicion_actual(0.0) {
    
    // Configurar pines de control del motor
    gpio_set_direction(step_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(dir_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(enable_pin, GPIO_MODE_OUTPUT);
    
    // --- ¡YA NO CONFIGURAMOS EL ADC AQUÍ! ---
}

// --- Método Principal de Movimiento ---
void Motor::activarMovimiento(float aceleracion, uint32_t direccion, int pasos_totales) {
    
    // 1. Validación de parámetros
    if (pasos_totales <= 0) return;

    // 2. Capar la aceleración solicitada al límite físico del motor
    if (aceleracion > accel_max) {
        aceleracion = accel_max;
    }
    // Asegurar que la aceleración no sea cero para evitar división por cero
    if (aceleracion <= 0) {
        return; // Se cancela el movimiento
    }

    // 3. Configurar dirección
    gpio_set_level(dir_pin, direccion);

    // =================================================================
    // INICIO: Planificación de Movimiento en Unidades Angulares
    // =================================================================

    // 4. Convertir 'pasos' de entrada a 'radianes'
    float despl_angular_total = (float)pasos_totales * (2.0 * M_PI) / (float)pasos_por_rev;

    // 5. Calcular velocidad máxima usable (en rad/s)
    //    Esta función ya capa la velocidad a 'vel_max'
    float vel_max_usable = calcularVelocidadMaxima(aceleracion, despl_angular_total);

    // 6. Calcular desplazamiento angular para la fase de aceleración (en rad)
    float despl_aceleracion = calcularDesplazamientoAceleracion(aceleracion, vel_max_usable);
    float despl_desaceleracion = despl_aceleracion; // perfil simétrico

    // 7. Calcular desplazamiento angular para la fase de velocidad constante
    float despl_vel_constante = despl_angular_total - despl_aceleracion - despl_desaceleracion;

    // 8. Ajustar perfil (Triangular vs Trapezoidal)
    if (despl_vel_constante < 0) {
        // Perfil Triangular: No hay tiempo de alcanzar vel_max_usable
        // La distancia es muy corta.
        despl_aceleracion = despl_angular_total / 2.0;
        despl_desaceleracion = despl_angular_total / 2.0;
        despl_vel_constante = 0;
        
        // Recalculamos la velocidad máxima que SÍ se alcanzará (en el pico del triángulo)
        vel_max_usable = sqrt(2.0 * aceleracion * despl_aceleracion);
    }
    
    // =================================================================
    // FIN: Planificación de Movimiento
    // =================================================================

    // 9. Convertir desplazamientos angulares (rad) de nuevo a pasos (int)
    int pasos_aceleracion = (int)(despl_aceleracion * (float)pasos_por_rev / (2.0 * M_PI));
    int pasos_desaceleracion = (int)(despl_desaceleracion * (float)pasos_por_rev / (2.0 * M_PI));
    // Ajustamos los pasos constantes para evitar errores de redondeo
    int pasos_vel_constante = pasos_totales - pasos_aceleracion - pasos_desaceleracion;


    // 10. Calcular Delays (en microsegundos)
    
    // Convertir velocidad máxima (rad/s) a (pasos/segundo)
    float vel_max_pasos = vel_max_usable * (float)pasos_por_rev / (2.0 * M_PI);

    // Calcular delay para la velocidad máxima (tiempo entre flancos del pulso)
    // delay = 1 / (frecuencia * 2) = 1 / (pasos_por_seg * 2)
    int delay_min = (int)(1000000.0 / (2.0 * vel_max_pasos));
    
    // Establecer un delay inicial más lento para la rampa de aceleración
    int delay_max = delay_min * 3; // (Valor ajustable, 3x más lento)

    // 11. Aplicar límites de seguridad a los delays
    if (delay_min < 100) delay_min = 100;   // Límite máx de frec: ~5000 pasos/seg
    if (delay_max > 5000) delay_max = 5000; // Límite mín de frec: ~100 pasos/seg
    if (delay_max < delay_min) delay_max = delay_min * 1.5; // Asegurar que max > min

    // 12. Ejecutar las tres fases del movimiento
    int delay_actual;

    // Fase 1: Aceleración (disminuir delay linealmente)
    for (int i = 0; i < pasos_aceleracion; i++) {
        // Interpola linealmente el delay
        delay_actual = delay_max - (long)(delay_max - delay_min) * i / pasos_aceleracion;
        ejecutarPaso(delay_actual);
    }

    // Fase 2: Velocidad constante
    for (int i = 0; i < pasos_vel_constante; i++) {
        ejecutarPaso(delay_min);
    }

    // Fase 3: Desaceleración (aumentar delay linealmente)
    for (int i = 0; i < pasos_desaceleracion; i++) {
        // Interpola linealmente el delay
        delay_actual = delay_min + (long)(delay_max - delay_min) * i / pasos_desaceleracion;
        ejecutarPaso(delay_actual);
    }

    // 13. Actualizar posición actual
    posicion_actual = leerPosicion(adc_channel_RESERVED);
}

// --- Funciones de Ayuda (Implementación) ---

float Motor::calcularVelocidadMaxima(float aceleracion, float despl_angular_total) {
    // Calcular velocidad máxima teórica basada en la distancia (v² = 2*a*s)
    float vel_max_teorica = sqrt(2.0 * aceleracion * despl_angular_total);
    
    // Devolver la velocidad más baja: la teórica o el límite físico del motor
    return fminf(vel_max_teorica, vel_max);
}

float Motor::calcularDesplazamientoAceleracion(float aceleracion, float vel_max_usable) {
    // Calcular distancia angular necesaria para alcanzar la vel_max_usable (s = v² / (2*a))
    return (vel_max_usable * vel_max_usable) / (2.0 * aceleracion);
}

float Motor::leerPosicion(adc1_channel_t channel) {
    // Lee el canal ADC específico que se le pasa
    int adc_val = adc1_get_raw(channel);
    
    // Mapeo corregido (de -135 a 135)
    float posicion_grados = (adc_val / 4095.0f) * 270.0f - 135.0f;
    
    // Actualizamos la variable interna
    this->posicion_actual = posicion_grados;
    
    return this->posicion_actual;
}

void Motor::ejecutarPaso(int delay_us) {
    // Generar un pulso cuadrado en el pin de 'step'
    gpio_set_level(step_pin, 1);
    esp_rom_delay_us(delay_us);
    gpio_set_level(step_pin, 0);
    esp_rom_delay_us(delay_us);
}

void Motor::habilitar() {
    gpio_set_level(enable_pin, 0);
}

void Motor::deshabilitar() {
    gpio_set_level(enable_pin, 1);
}