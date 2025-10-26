#ifndef MONITORSERIAL_H
#define MONITORSERIAL_H

#include "Motor.h" // Necesita saber qué es un "Motor"

class MonitorSerial {
public:
    MonitorSerial();
    
    /**
     * @brief Inicializa el monitor. En ESP-IDF, la UART ya está
     * inicializada por defecto, así que solo imprime un saludo.
     */
    void iniciarMonitor();

    /**
     * @brief Esta es la función principal del Núcleo 1.
     * Imprime la tabla de estado usando printf.
     */
    void actualizarDatos(Motor* motores[3]);

    /**
     * @brief Recibe las direcciones desde el Núcleo 0 (Exobrazo)
     * para mostrarlas de forma segura.
     */
    void actualizarDirecciones(int* dirEstado);

private:
    // Copia segura de las direcciones para evitar conflictos entre núcleos
    int dirEstadosCopia[3]; 
};

#endif // MONITORSERIAL_H