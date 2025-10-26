#ifndef DEFINES_H
#define DEFINES_H

#include <driver/gpio.h>
#include <driver/adc.h>

// --- PINES DE MOTORES ---
#define STEP_PIN_1  GPIO_NUM_12 // Motor 1
#define DIR_PIN_1   GPIO_NUM_14
#define ADC_PIN_1   ADC1_CHANNEL_7 // GPIO 35 (potPins[0])

#define STEP_PIN_2  GPIO_NUM_16 // Motor 2
#define DIR_PIN_2   GPIO_NUM_17
#define ADC_PIN_2   ADC1_CHANNEL_4 // GPIO 32 (potPins[1])

#define STEP_PIN_3  GPIO_NUM_13 // Motor 3
#define DIR_PIN_3   GPIO_NUM_15
#define ADC_PIN_3   ADC1_CHANNEL_5 // GPIO 33 (potPins[2])

// --- PINES DE SEÑALES (Basado en tu .ino de prueba) ---
#define SIGNAL_PIN_1  GPIO_NUM_36 // Botón individual 1 (individualButtonPins[0])
#define SIGNAL_PIN_2  GPIO_NUM_39 // Botón individual 2 (individualButtonPins[1])
#define SIGNAL_PIN_3  GPIO_NUM_34 // Botón individual 3 (individualButtonPins[2])
#define GSIGNAL_PIN   GPIO_NUM_2  // Botón General (buttonPin)

// --- LÍMITES FÍSICOS (Basado en tu .ino de prueba) ---
#define LIM_MIN_M1    -40.0f
#define LIM_MAX_M1    130.0f
#define LIM_MIN_M2    -100.0f
#define LIM_MAX_M2    80.0f
#define LIM_MIN_M3    0.0f    // Límite "mínimo" (lógica invertida)
#define LIM_MAX_M3    -100.0f // Límite "máximo" (lógica invertida)

// --- PARÁMETROS DE DATASHEET ---

// Motor 1 (42SHDC3025-24B)
// Datasheet: 1.8° por paso => 360 / 1.8 = 200 pasos/rev
#define PASOS_REV_M1  200.0f 
#define ACCEL_MAX_M1  50.0f  // (rad/s²) Estimación segura
#define VEL_MAX_M1    104.0f // (rad/s) 1000rpm

// Motor 2 y 3 (24BYJ48)
// Datasheet: 5.625° por paso y reductora 1:64
// => (360 / 5.625) * 64 = 4096 pasos/rev
#define PASOS_REV_M2  4096.0f
#define ACCEL_MAX_M2  1.5f   // (rad/s²) Estimación segura
#define VEL_MAX_M2    0.8f   // (rad/s) Estimación (5V)

#define PASOS_REV_M3  4096.0f
#define ACCEL_MAX_M3  1.5f   // (rad/s²)
#define VEL_MAX_M3    0.8f   // (rad/s)

// --- PARÁMETROS DE PRUEBA (Rebote) ---
// Aceleración y pasos ESTÁNDAR para la prueba de botones
#define ACCEL_PRUEBA_STD  2.0f  // Aceleración estándar (ej: 2.0 rad/s²)
#define PASOS_PRUEBA_STD  50    // Pasos por movimiento (ej: 50 pasos)

#endif // DEFINES_H