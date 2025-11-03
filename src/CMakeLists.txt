# 1. Definimos una variable con TODOS tus archivos .cpp
set(COMPONENT_SRCS
    "main.cpp"
    "Exobrazo.cpp"
    "Motor.cpp"
    "Signal.cpp"
    "GSignal.cpp"
    "MonitorSerial.cpp"
)

# 2. Definimos la ruta a tus archivos .h (subiendo un nivel desde src a include)
set(COMPONENT_INCLUDE_DIRS
    "../include"
)

# 3. Registramos el componente
idf_component_register(SRCS ${COMPONENT_SRCS}
                      INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
                      REQUIRES
                          freertos
                          esp_wifi
                          esp_netif
                          nvs_flash
                          esp_http_server
                          json
)