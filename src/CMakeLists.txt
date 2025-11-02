# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS  ${app_sources} "main.cpp" "Exobrazo.cpp" "Motor.cpp" "Signal.cpp" "GSignal.cpp" "MonitorSerial.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES 
                           freertos            # Para las tareas y colas
                           esp_wifi            # Para el Wi-Fi
                           esp_netif           # Para networking
                           nvs_flash           # Para guardar el Wi-Fi
                           esp_http_server     # El servidor web
                           cjson               # Para manejar JSON
)