#ifndef MONITORSERIAL_H
#define MONITORSERIAL_H

#include "Motor.h" // Necesita saber qué es un "Motor"

class MonitorSerial {
public:
    MonitorSerial();
    void iniciarMonitor();
    void actualizarDatos(Motor* motores[3]);
    void actualizarDirecciones(int* dirEstado);

    /**
     * @brief (NUEVO) Recibe el estado actual de los botones
     * desde el Núcleo 0 de forma segura.
     * @param estadosBotones Un arreglo de 3 booleanos (true=presionado).
     */
    void actualizarBotones(bool* estadosBotones);

private:
    int dirEstadosCopia[3];
    
    /**
     * @brief (NUEVO) Almacenamiento seguro para el estado de los botones.
     */
    bool botonesCopia[3];
};

#endif // MONITORSERIAL_H