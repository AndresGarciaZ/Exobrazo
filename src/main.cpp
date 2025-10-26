/*
 * Archivo: main.cpp
 * Punto de entrada principal del firmware (nativo de ESP-IDF)
 */

#include "Exobrazo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- 1. Objeto Principal Global ---
Exobrazo exobrazo;

// --- 2. Tareas de FreeRTOS ---

/**
 * @brief Tarea del Monitor Serial (se ejecuta en Núcleo 1)
 */
void monitorTask(void *pvParameters) {
    printf("Iniciando 'monitorTask' en Núcleo 1...\n");
    exobrazo.monitor.iniciarMonitor();

    // Bucle infinito para esta tarea
    for (;;) {
        exobrazo.monitor.actualizarDatos(exobrazo.motores);
        vTaskDelay(100 / portTICK_PERIOD_MS); // Refresca 10 veces/seg
    }
}

/**
 * @brief Tarea de Lógica Principal (se ejecuta en Núcleo 0)
 */
void mainLogicTask(void *pvParameters) {
    printf("Iniciando 'mainLogicTask' en Núcleo 0...\n");

    // Bucle infinito para la lógica principal
    for (;;) {
        // 1. El Núcleo 0 ejecuta la lógica (rebote, etc.)
        exobrazo.ejecutarCicloPrincipal();
        
        // 2. (OPCIONAL pero recomendado) Publicar el estado
        // de las direcciones al monitor de forma segura.
        // exobrazo.monitor.actualizarDirecciones(exobrazo.getDirEstadosPrueba());
        
        vTaskDelay(1 / portTICK_PERIOD_MS); // Pausa mínima
    }
}


// --- 3. Punto de Entrada Principal (app_main) ---

// extern "C" es necesario para el punto de entrada de C++
extern "C" void app_main(void) {
    
    printf("Iniciando app_main()...\n");
    
    // 1. Inicializar el objeto Exobrazo
    // (Esto vincula GSignal a las Signals)
    exobrazo.iniciar();

    // 2. Crear la Tarea del Monitor en el Núcleo 1
    xTaskCreatePinnedToCore(
        monitorTask,          // Función de la tarea
        "MonitorTask",        // Nombre de la tarea
        4096,                 // Tamaño de la pila
        NULL,                 // Parámetros
        1,                    // Prioridad (baja)
        NULL,                 // Handle (no necesitamos guardarlo)
        1                     // Núcleo 1
    );

    // 3. Crear la Tarea de Lógica Principal en el Núcleo 0
    xTaskCreatePinnedToCore(
        mainLogicTask,        // Función de la tarea
        "MainLogicTask",      // Nombre de la tarea
        4096,                 // Tamaño de la pila
        NULL,                 // Parámetros
        5,                    // Prioridad (más alta que el monitor)
        NULL,                 // Handle
        0                     // Núcleo 0
    );

    // app_main() termina aquí, pero las tareas que ha creado
    // (monitorTask y mainLogicTask) seguirán ejecutándose.
}