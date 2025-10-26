#include "MonitorSerial.h"
#include <stdio.h> // Para printf
#include "freertos/FreeRTOS.h" // Para FreeRTOS
#include "freertos/task.h"      // Para vTaskDelay
#include "defines.h" 

MonitorSerial::MonitorSerial() {
    dirEstadosCopia[0] = 1;
    dirEstadosCopia[1] = 1;
    dirEstadosCopia[2] = 1;
}

void MonitorSerial::iniciarMonitor() {
    // En ESP-IDF, la UART para el monitor ya está configurada.
    // Solo esperamos un momento y enviamos un saludo.
    vTaskDelay(100 / portTICK_PERIOD_MS);
    printf("\n\n=== SISTEMA DE CONTROL DE MOTORES ===\n");
    printf("Monitor Serial iniciado en Núcleo 1.\n");
}

void MonitorSerial::actualizarDirecciones(int* dirEstado) {
    // Copia los valores del Núcleo 0 al almacenamiento local del Núcleo 1
    for(int i=0; i<3; i++) {
        dirEstadosCopia[i] = dirEstado[i];
    }
}

void MonitorSerial::actualizarDatos(Motor* motores[3]) {
    
    // Borra pantalla
    printf("\033[0H\033[0J");
    printf("=== SISTEMA DE CONTROL DE MOTORES ===\n");
    printf("--------------------------------------\n");
    
    printf("INFORMACIÓN DE LOS MOTORES (NÚCLEO 1):\n");
    printf("---------------------------\n");
    printf("Motor | Ángulo (Stale) | Dirección | Pasos/Rev | Límites\n");
    printf("----------------------------------------------------------------\n");

    for (int i = 0; i < 3; i++) {
        // Obtenemos datos de forma segura (Getters)
        // Esta posición es "stale" - la última conocida por el Núcleo 0
        float angulo = motores[i]->getPosicionActual(); 
        int dir = dirEstadosCopia[i]; // Lee la copia segura
        float minLim = motores[i]->getLimiteMin();
        float maxLim = motores[i]->getLimiteMax();
        int pasosRev = motores[i]->getPasosPorRev();

        // Imprimir con formato usando printf
        printf("  %d   | ", i + 1);
        printf("%s%.1f°       | ", (angulo >= 0 ? " " : ""), angulo);
        printf("%s  | ", (dir == -1 ? "Horaria  " : "Antihor. "));
        printf("%-8d  | ", pasosRev);
        printf("%.0f° a %.0f°", minLim, maxLim);
        
        // Lógica de límites
        bool atMinLimit = false;
        bool atMaxLimit = false;
        if (minLim > maxLim) { // Invertida M3
            atMinLimit = (angulo >= minLim);
            atMaxLimit = (angulo <= maxLim);
        } else { // Normal M1, M2
            atMinLimit = (angulo <= minLim);
            atMaxLimit = (angulo >= maxLim);
        }

        if (atMinLimit) {
            printf(" - LÍMITE MÍN\n");
        } else if (atMaxLimit) {
            printf(" - LÍMITE MÁX\n");
        } else {
            printf("\n");
        }
    }
    printf("\n--------------------------------------\n");
}