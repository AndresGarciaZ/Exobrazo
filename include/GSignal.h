#ifndef GSIGNAL_H
#define GSIGNAL_H

#include <driver/gpio.h>
#include "Signal.h"

/**
 * @class GSignal
 * @brief Actúa como un "interruptor maestro" o "botón general".
 *
 * Detecta la pulsación (flanco de subida) y liberación (flanco de bajada)
 * de un pin de entrada.
 *
 * Cuando se activa, "fuerza" el estado lógico de todas las señales (Signal)
 * que tiene registradas.
 */
class GSignal {
private:
    // --- Hardware ---
    gpio_num_t pin; // El pin físico del botón general

    // --- Detección de Flancos (Edge Detection) ---
    bool estado;          // Estado actual del pin (leído en el último ciclo)
    bool estado_anterior; // Estado del pin en el ciclo anterior

    // --- Gestión de Señales ---
    Signal* signals[3]; // Array de punteros a las señales de los motores
    int numSignals;     // Contador de cuántas señales se han agregado

    // --- Estado Lógico (Latch) ---
    /**
     * @brief Es un 'latch' o pestillo. Se vuelve 'true' cuando se pulsa el botón
     * y 'false' cuando se suelta. Permanece en ese estado.
     */
    bool gsignal_activo;

public:
    /**
     * @brief Constructor.
     * @param pin El pin GPIO que se usará como entrada.
     */
    GSignal(gpio_num_t pin);

    /**
     * @brief Registra una señal (de un motor) para ser controlada por este botón.
     * @param signal Puntero al objeto Signal que se desea controlar.
     */
    void agregarSignal(Signal* signal);

    /**
     * @brief Obtiene el estado del pin leído en el último ciclo.
     * (Nota: Es preferible usar estaActivo() para saber si el latch está puesto).
     */
    bool obtenerEstado();

    /**
     * @brief El corazón de la clase. Debe llamarse en cada ciclo del loop principal.
     * Lee el pin y comprueba si ha habido un cambio (flanco) para actuar.
     */
    void actualizarEstado();

    /**
     * @brief Devuelve el estado del 'latch' (pestillo).
     * @return true si el botón general ha sido activado y no liberado.
     */
    bool estaActivo() const { return gsignal_activo; }

private:
    /**
     * @brief Llama a signal->establecerEstado(true) en todas las señales registradas.
     * Esto "fuerza" su estado lógico a 'activo'.
     */
    void forzarActivacionTodas();

    /**
     * @brief Llama a signal->establecerEstado(false) en todas las señales registradas.
     * Esto "libera" su estado lógico a 'inactivo'.
     */
    void liberarTodas();

    /**
     * @brief Comprueba si el botón acaba de ser presionado (flanco de subida).
     * @return true si el estado anterior era 'false' y el actual es 'true'.
     */
    bool haCambiadoAActivo();

    /**
     * @brief Comprueba si el botón acaba de ser soltado (flanco de bajada).
     * @return true si el estado anterior era 'true' y el actual es 'false'.
     */
    bool haCambiadoAInactivo();
};

#endif // GSIGNAL_H