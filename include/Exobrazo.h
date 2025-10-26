#ifndef EXOBRAZO_H
#define EXOBRAZO_H

#include "Motor.h"
#include "Signal.h"
#include "GSignal.h"
#include "MonitorSerial.h"
#include "defines.h"

/**
 * @class Exobrazo
 * @brief El "Cerebro" de la aplicación.
 * Contiene la lógica principal, coordina los motores y señales,
 * y gestiona los modos de control (Prueba vs Web).
 */
class Exobrazo {
public:
    // --- Componentes Públicos ---
    // (Públicos para que la Tarea del Núcleo 1 pueda leerlos)
    MonitorSerial monitor;
    Motor* motores[3];

    // --- Métodos Principales ---
    Exobrazo();
    void iniciar(); // Configuración secundaria
    
    /**
     * @brief Bucle principal de lógica (se ejecuta en Núcleo 0).
     */
    void ejecutarCicloPrincipal();

    // --- API DE CONTROL (para la Web) ---
    
    /**
     * @brief (HUECO) Punto de entrada para comandos web.
     * Toma el control del sistema (modo WEB).
     */
    void moverMotorWeb(int motorIndex, float aceleracion, uint32_t direccion, int pasos);

    /**
     * @brief (HUECO) Devuelve el control a los botones físicos (modo PRUEBA).
     */
    void setModoPrueba();


private:
    // --- Componentes Privados (Encapsulados) ---
    Signal signalMotor1;
    Signal signalMotor2;
    Signal signalMotor3;
    
    Motor motor1;
    Motor motor2;
    Motor motor3;
    
    GSignal senalGeneral;

    // --- Estado de la Aplicación ---
    enum class ModoControl { PRUEBA, WEB };
    ModoControl modoActual;
    
    /**
     * @brief Almacena la dirección de rebote (1 o -1) para el modo PRUEBA.
     * (Equivalente a dirStates[3] de tu .ino)
     */
    int dirEstadosPrueba[3];
};

#endif // EXOBRAZO_H