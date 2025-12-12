/*
 * Archivo: main.cpp
 * Punto de entrada principal del firmware (nativo de ESP-IDF)
 * Incluye lógica de Wi-Fi, Servidor HTTP (Núcleo 1) y Cola (Queue)
 */

#include "Exobrazo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Includes para Wi-Fi y Servidor
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"

// --- Configuración de Wi-Fi ---

#define WIFI_SSID "Exobrazito"
#define WIFI_PASSWORD "Embebid2025"
static const char *TAG_WIFI = "wifi_station";

// --- 1. Objeto Principal Global y Cola ---
Exobrazo exobrazo;
QueueHandle_t webCommandQueue; // La "Cola" (buzón) entre núcleos

// Estructura para los mensajes de la cola
typedef struct {
    enum class CommandType { MOVE, SET_MODE } type;
    // Para MOVE
    int motorIndex;
    float aceleracion;
    uint32_t direccion;
    int pasos;
    // Para SET_MODE
    Exobrazo::ModoControl modo;
} WebCommand;


// --- 2. Handlers del Servidor HTTP (Se ejecutan en Núcleo 1) ---

// Handler para POST /move
esp_err_t move_handler(httpd_req_t *req) {
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
     if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
         httpd_resp_set_status(req, "400 Bad Request");
         httpd_resp_set_type(req, "text/plain");
         httpd_resp_send(req, "JSON invalido", strlen("JSON invalido"));
         return ESP_FAIL;
    }

    WebCommand cmd;
    cmd.type = WebCommand::CommandType::MOVE;
    cmd.motorIndex = cJSON_GetObjectItem(json, "motorIndex")->valueint;
    cmd.aceleracion = cJSON_GetObjectItem(json, "aceleracion")->valuedouble;
    cmd.direccion = cJSON_GetObjectItem(json, "direccion")->valueint;
    cmd.pasos = cJSON_GetObjectItem(json, "pasos")->valueint;

    cJSON_Delete(json);
    
    // Enviar el comando a la cola para que el Núcleo 0 lo procese
    if (xQueueSend(webCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "Error procesando comando", strlen("Error procesando comando"));
        return ESP_FAIL;
    }
    
    // Responder al frontend
    const char* resp = "{\"status\": \"ok\", \"message\": \"Comando de movimiento recibido\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Habilitar CORS
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

// Handler para POST /setMode
esp_err_t set_mode_handler(httpd_req_t *req) {
    char buf[64];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "JSON invalido", strlen("JSON invalido"));
        return ESP_FAIL;
    }

WebCommand cmd;
cmd.type = WebCommand::CommandType::SET_MODE;
const char* mode_str = cJSON_GetObjectItem(json, "mode")->valuestring;

if (strcmp(mode_str, "web") == 0) {
    cmd.modo = Exobrazo::ModoControl::WEB;
} else {
    cmd.modo = Exobrazo::ModoControl::PRUEBA;
}

cJSON_Delete(json);

// Enviar el comando a la cola
xQueueSend(webCommandQueue, &cmd, pdMS_TO_TICKS(100));

// Responder
char resp_buf[64];
sprintf(resp_buf, "{\"status\": \"ok\", \"currentMode\": \"%s\"}", mode_str);
httpd_resp_set_type(req, "application/json");
httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Habilitar CORS
httpd_resp_send(req, resp_buf, strlen(resp_buf));
return ESP_OK;
}

// Handler para GET /status
esp_err_t status_handler(httpd_req_t *req) {
// Este handler (Núcleo 1) lee de forma segura los datos de 'exobrazo'
// (que son actualizados por el Núcleo 0)
cJSON *root = cJSON_CreateObject();

if (exobrazo.getModoActual() == Exobrazo::ModoControl::WEB) {
cJSON_AddStringToObject(root, "mode", "web");
} else {
    cJSON_AddStringToObject(root, "mode", "prueba");
}

cJSON *motores = cJSON_CreateArray();
cJSON_AddItemToObject(root, "motores", motores);

for (int i = 0; i < 3; i++) {
    cJSON *motor = cJSON_CreateObject();
    cJSON_AddNumberToObject(motor, "id", i);
    // Leemos la posición actual (última conocida)
    cJSON_AddNumberToObject(motor, "posicion", exobrazo.motores[i]->getPosicionActual()); 
    cJSON_AddItemToArray(motores, motor);
}

char *json_str = cJSON_PrintUnformatted(root);
httpd_resp_set_type(req, "application/json");
httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // Habilitar CORS
httpd_resp_send(req, json_str, strlen(json_str));

free(json_str);
cJSON_Delete(root);
return ESP_OK;
}

// Handler para CORS (necesario para que el navegador permita la conexión)
esp_err_t cors_options_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}


// --- 3. Tareas de FreeRTOS ---

void monitorTask(void *pvParameters) {
    printf("Iniciando 'monitorTask' en Núcleo 1...\n");
    exobrazo.monitor.iniciarMonitor();
    for (;;) {
        exobrazo.monitor.actualizarDatos(exobrazo.motores);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// NUEVA TAREA: Servidor Web (Núcleo 1)
void webServerTask(void *pvParameters) {
    printf("Iniciando 'webServerTask' en Núcleo 1...\n");

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 1; // Fijar al Núcleo 1

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE("HTTPD", "Error al iniciar el servidor");
        vTaskDelete(NULL);
        return;
    }

    // Configurar Endpoints
    httpd_uri_t move_uri = {
        .uri = "/move", .method = HTTP_POST, .handler = move_handler, .user_ctx = NULL
    };
    
    httpd_register_uri_handler(server, &move_uri);
    httpd_uri_t set_mode_uri = {
        .uri = "/setMode", .method = HTTP_POST, .handler = set_mode_handler, .user_ctx = NULL
    };
    
    httpd_register_uri_handler(server, &set_mode_uri);
    
    httpd_uri_t status_uri = {
        .uri = "/status", .method = HTTP_GET, .handler = status_handler, .user_ctx = NULL
    };
    
    httpd_register_uri_handler(server, &status_uri);

    // Handler para OPTIONS (CORS)
    httpd_uri_t move_options_uri = {
        .uri = "/move", .method = HTTP_OPTIONS, .handler = cors_options_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &move_options_uri);
    httpd_uri_t setmode_options_uri = {
        .uri = "/setMode", .method = HTTP_OPTIONS, .handler = cors_options_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &setmode_options_uri);
    
    ESP_LOGI("HTTPD", "Servidor iniciado en puerto 80");
    
    // La tarea se mantiene viva aquí
    for(;;) { vTaskDelay(portMAX_DELAY); }
}

// TAREA MODIFICADA: Lógica Principal (Núcleo 0)
void mainLogicTask(void *pvParameters) {
    printf("Iniciando 'mainLogicTask' en Núcleo 0...\n");
    WebCommand receivedCommand;
    
    for (;;) {
        // 1. Revisar si hay un comando de la web en la cola
        if (xQueueReceive(webCommandQueue, &receivedCommand, 0) == pdTRUE) {
            // Si hay comando, procesarlo
            if (receivedCommand.type == WebCommand::CommandType::MOVE) {
                // La web siempre fuerza el modo WEB
                exobrazo.setModoWeb(); 
                exobrazo.moverMotorWeb(receivedCommand.motorIndex, receivedCommand.aceleracion, receivedCommand.direccion, receivedCommand.pasos);
            } else if (receivedCommand.type == WebCommand::CommandType::SET_MODE) {
                if (receivedCommand.modo == Exobrazo::ModoControl::PRUEBA) {
                    exobrazo.setModoPrueba();
                } else {
                    exobrazo.setModoWeb(); // Asumiendo que este método existe
                    }
                }
            } else {
            // 2. Si no hay comandos web, ejecutar la lógica de botones
            exobrazo.ejecutarCicloPrincipal();
        }
        
        vTaskDelay(1 / portTICK_PERIOD_MS); // Pausa mínima
        }
}

// --- 4. Funciones de Wi-Fi ---
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG_WIFI, "Reconectando a Wi-Fi...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "¡Conectado! IP Obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Inicializar Wi-Fi en modo AP (punto de acceso)
void wifi_init_ap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, WIFI_SSID);
    wifi_config.ap.ssid_len = strlen(WIFI_SSID);
    strcpy((char*)wifi_config.ap.password, WIFI_PASSWORD);
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen(WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "WiFi AP iniciado. SSID:%s password:%s", WIFI_SSID, WIFI_PASSWORD);
}

/*
void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));
    
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG_WIFI, "wifi_init_sta finalizado.");
}
*/
// --- 5. Punto de Entrada Principal (app_main) ---
extern "C" void app_main(void) {
    
    printf("Iniciando app_main()...\n");
    
    // --- 1. Inicializar NVS (Necesario para Wi-Fi) ---
    esp_err_t ret = nvs_flash_init();
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);
    
    // --- 2. Conectar a Wi-Fi ---
    wifi_init_ap(); // Modo Punto de Acceso
    
    // --- 3. Inicialización Global de ADC (Tu código original) ---
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten((adc1_channel_t)ADC_CHANNEL_7, ADC_ATTEN_DB_11);
    adc1_config_channel_atten((adc1_channel_t)ADC_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten((adc1_channel_t)ADC_CHANNEL_5, ADC_ATTEN_DB_11);
    
    // --- 4. Crear la Cola (Buzón) ---
    // Creamos una cola para 5 comandos
    webCommandQueue = xQueueCreate(5, sizeof(WebCommand));
    
    // --- 5. Inicializar el objeto Exobrazo ---
    exobrazo.iniciar();
    
    // --- 6. Crear Tareas ---
    // Tarea del Monitor en Núcleo 1
    xTaskCreatePinnedToCore(
        monitorTask, "MonitorTask", 4096, NULL, 1, NULL, 1
    );
    
    // Tarea de Lógica Principal en Núcleo 0
    xTaskCreatePinnedToCore(
        mainLogicTask, "MainLogicTask", 4096, NULL, 5, NULL, 0
    );
    
    // NUEVA:Tarea del Servidor web en Núcleo 1
    xTaskCreatePinnedToCore(
        webServerTask, "WebServerTask", 8192, NULL, 5, NULL, 1
    );

     printf("app_main() finalizado. Las tareas estan corriendo.\n");
}

