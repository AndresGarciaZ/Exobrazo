#include "MonitorSerial.h"
#include <stdio.h> // Para printf
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "defines.h"

MonitorSerial::MonitorSerial() {
    // Inicializar estados por defecto
    dirEstadosCopia[0] = 1;
    dirEstadosCopia[1] = 1;
    dirEstadosCopia[2] = 1;
    botonesCopia[0] = false;
    botonesCopia[1] = false;
    botonesCopia[2] = false;
}

void MonitorSerial::iniciarMonitor() {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    printf("\n\n=== SISTEMA DE CONTROL DE MOTORES ===\n");
    printf("Monitor Serial iniciado en Núcleo 1.\n");
}

void MonitorSerial::actualizarDirecciones(int* dirEstado) {
    for(int i=0; i<3; i++) {
        dirEstadosCopia[i] = dirEstado[i];
    }
}

/**
 * @brief (NUEVO) Copia el estado de los botones recibido del Núcleo 0.
 */
void MonitorSerial::actualizarBotones(bool* estadosBotones) {
    for(int i=0; i<3; i++) {
        botonesCopia[i] = estadosBotones[i];
    }
}


void MonitorSerial::actualizarDatos(Motor* motores[3]) {
    
    printf("\033[0H\033[0J"); // Borra pantalla
    printf("=== SISTEMA DE CONTROL DE MOTORES ===\n");
    printf("--------------------------------------\n");
    
    printf("INFORMACIÓN DE LOS MOTORES (NÚCLEO 1):\n");
    printf("---------------------------\n");
    // --- CABECERA MODIFICADA ---
    printf("Motor | Ángulo (Stale) | Dirección | Pasos/Rev | Límites        | Botón\n");
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < 3; i++) {
        float angulo = motores[i]->getPosicionActual(); 
        int dir = dirEstadosCopia[i];
        float minLim = motores[i]->getLimiteMin();
        float maxLim = motores[i]->getLimiteMax();
        int pasosRev = motores[i]->getPasosPorRev();
        bool botonPresionado = botonesCopia[i]; // Lee la copia segura

        printf("  %d   | ", i + 1);
        printf("%s%.1f°       | ", (angulo >= 0 ? " " : ""), angulo);
        printf("%s  | ", (dir == -1 ? "Horaria  " : "Antihor. "));
        printf("%-8d  | ", pasosRev);
        printf("%-13s |", " "); // Espacio reservado para límites

        // Imprimir límites en la misma línea
        char limStr[20];
        snprintf(limStr, sizeof(limStr), "%.0f a %.0f", minLim, maxLim);
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b%-13s |", limStr); // \b para retroceder y sobrescribir

        // --- IMPRIMIR ESTADO DEL BOTÓN ---
        printf(" %s", botonPresionado ? "ON \n" : "OFF\n");

        // (La lógica de imprimir "LÍMITE MÍN/MÁX" se puede añadir aquí si se desea)
        // ...
    }
    printf("\n--------------------------------------\n");
    // (Puedes añadir aquí el estado del botón general si lo pasas también)
}