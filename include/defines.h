#ifndef DEFINES_H
#define DEFINES_H

#include <driver/gpio.h>
#include <driver/adc.h>

// --- PINES DE MOTORES ---
#define STEP_PIN_1  GPIO_NUM_23 // Motor 1
#define DIR_PIN_1   GPIO_NUM_22
#define ENABLE_PIN_1 GPIO_NUM_21 // Habilitación del motor 1
#define ADC_PIN_1   ADC1_CHANNEL_7 // GPIO 35 (potPins[0])

#define STEP_PIN_2  GPIO_NUM_16 // Motor 2
#define DIR_PIN_2   GPIO_NUM_17
#define ENABLE_PIN_2 GPIO_NUM_5 // Habilitación del motor 2
#define ADC_PIN_2   ADC1_CHANNEL_4 // GPIO 32 (potPins[1])

#define STEP_PIN_3  GPIO_NUM_18 // Motor 3
#define DIR_PIN_3   GPIO_NUM_19
#define ENABLE_PIN_3 GPIO_NUM_4 // Habilitación del motor 3
#define ADC_PIN_3   ADC1_CHANNEL_5 // GPIO 33 (potPins[2])

// --- PINES DE SEÑALES (Basado en tu .ino de prueba) ---
#define SIGNAL_PIN_1  GPIO_NUM_36 // Botón individual 1 (individualButtonPins[0])
#define SIGNAL_PIN_2  GPIO_NUM_39 // Botón individual 2 (individualButtonPins[1])
#define SIGNAL_PIN_3  GPIO_NUM_34 // Botón individual 3 (individualButtonPins[2])
#define GSIGNAL_PIN   GPIO_NUM_2  // Botón General (buttonPin)

// --- LÍMITES FÍSICOS (Basado en tu .ino de prueba) ---
#define LIM_MIN_M1    23.0f
#define LIM_MAX_M1    -110.0f
#define LIM_MIN_M2    135.0f
#define LIM_MAX_M2    -92.0f
#define LIM_MIN_M3    0.0f    // Límite "mínimo" (lógica invertida)
#define LIM_MAX_M3    135.0f // Límite "máximo" (lógica invertida)

// --- PARÁMETROS DE DATASHEET ---
// los 3 motores son de 1.8° por paso (200 pasos/rev) con microstepping 1/20 -> 4000 pasos/rev
// Motor 1 (42SHDC3025-24B)
#define PASOS_REV_M1  4000.0f 
#define ACCEL_MAX_M1  4.0f  // (rad/s²) Estimación segura
#define VEL_MAX_M1    1.1f // (rad/s) 10rpm

// Motor 2 (nema 17, 0.33Nm)
#define PASOS_REV_M2  4000.0f 
#define ACCEL_MAX_M2  4.0f   // (rad/s²) Estimación segura
#define VEL_MAX_M2    1.1f   // (rad/s) Estimación (5V)
// Motor 3  (nema 17, 0.33Nm)
#define PASOS_REV_M3  4000.0f
#define ACCEL_MAX_M3  4.0f   // (rad/s²)
#define VEL_MAX_M3    1.1f   // (rad/s)

// --- PARÁMETROS DE PRUEBA (Rebote) ---
#define ACCEL_PRUEBA_STD  3.0f  // Aceleración estándar
#define ANGULO_PRUEBA_STD 20.0f // El incremento en GRADOS

#endif // DEFINES_H