#ifndef MOTOR_H
#define MOTOR_H

#include <driver/gpio.h>
#include <driver/adc.h>
#include "Signal.h" 

class Motor {
private:
    // --- Pines (Hardware) ---
    gpio_num_t step_pin;
    gpio_num_t dir_pin;
    adc1_channel_t adc_channel; // Pin del potenciómetro

    // --- Parámetros físicos ---
    float accel_max;     // Aceleración máxima (rad/s²)
    float vel_max;       // Velocidad máxima (rad/s)
    int   pasos_por_rev; // Pasos por revolución (ej: 200, 400)

    // --- Límites y Estado ---
    float posicion_actual; // Posición actual en grados
    float limite_min;      // Límite en grados
    float limite_max;      // Límite en grados

//Considerar que todo se maneja en movimientos angulares, los pasos se convierten a grados en las funciones.

public:
    /**
     * @brief Referencia pública a la señal del motor.
     * Es pública para que la clase Exobrazo pueda leer su estado
     * (como se indica en el diagrama UML).
     */
    Signal& signal;

    /**
     * @brief Constructor de la clase Motor.
     * (Tus parámetros originales están correctos)
     */
    Motor(gpio_num_t step_pin, gpio_num_t dir_pin,
          adc1_channel_t adc_channel, Signal& signal,
          float accel_max, float vel_max, int pasos_por_rev,
          float limite_min, float limite_max);

    /**
     * @brief Activa un movimiento con un perfil de velocidad trapezoidal.
     */
    void activarMovimiento(float aceleracion, uint32_t direccion, int pasos);

    /**
     * @brief Lee la posición actual del motor desde el potenciómetro.
     * @return float Posición actual en grados.
     */
    float leerPosicion();

    // --- Getters 
    float getLimiteMin() const { return limite_min; }
    float getLimiteMax() const { return limite_max; }
    float getPosicionActual() const { return posicion_actual; }
    int getPasosPorRev() const { return pasos_por_rev; }

private:
    /**
     * @brief Ejecuta un único pulso (paso) en el pin de 'step'.
     */
    void ejecutarPaso(int delay_us);

    /**
     * @brief Calcula la velocidad angular máxima utilizable (rad/s).
     * @param aceleracion Aceleración angular (rad/s²)
     * @param despl_angular_total Desplazamiento angular total (rad)
     * @return float Velocidad máxima utilizable (rad/s)
     */
    float calcularVelocidadMaxima(float aceleracion, float despl_angular_total);

    /**
     * @brief Calcula el desplazamiento angular (rad) para la fase de aceleración.
     * @param aceleracion Aceleración angular (rad/s²)
     * @param vel_max_usable Velocidad angular máxima (rad/s)
     * @return float Desplazamiento angular de aceleración (rad)
     */
    float calcularDesplazamientoAceleracion(float aceleracion, float vel_max_usable);
};

#endif // MOTOR_H