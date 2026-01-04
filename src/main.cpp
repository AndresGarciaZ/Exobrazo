#include "Exobrazo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <esp_task_wdt.h>
#include <string.h>

// Includes para Wi-Fi y Servidor
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"

static const char *TAG = "EXOBRAZO_WEB";

Exobrazo exobrazo;
QueueHandle_t webCommandQueue;

typedef struct {
    enum class CommandType { MOVE, SET_MODE, THERAPEUTIC } type;
    int motorIndex;
    float velocidad; 
    uint32_t direccion;
    float angulo; // CAMBIADO: De int pasos a float angulo para grados decimales
    Exobrazo::ModoControl modo;
} WebCommand;

// --- SOPORTE CORS ---
void set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

esp_err_t cors_options_handler(httpd_req_t *req) {
    set_cors_headers(req);
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// --- HANDLERS HTTP ---

esp_err_t move_handler(httpd_req_t *req) {
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (!json) return ESP_FAIL;

    WebCommand cmd;
    cmd.type = WebCommand::CommandType::MOVE;
    cmd.motorIndex = cJSON_GetObjectItem(json, "motorIndex")->valueint;
    cmd.velocidad = (float)cJSON_GetObjectItem(json, "aceleracion")->valuedouble;
    cmd.direccion = cJSON_GetObjectItem(json, "direccion")->valueint;
    // CAPTURAMOS EL ÁNGULO: La web envía el valor decimal en el campo "pasos"
    cmd.angulo = (float)cJSON_GetObjectItem(json, "pasos")->valuedouble; 
    cJSON_Delete(json);

    // Validación de límites (usando los grados directamente)
    float pos = exobrazo.motores[cmd.motorIndex]->getPosicionActual();
    bool bloqueado = (cmd.direccion == 1 && pos >= exobrazo.motores[cmd.motorIndex]->getLimiteMax()) || 
                     (cmd.direccion == 0 && pos <= exobrazo.motores[cmd.motorIndex]->getLimiteMin());

    set_cors_headers(req);
    if (bloqueado) {
        char resp[128];
        snprintf(resp, sizeof(resp), "{\"status\": \"error\", \"reason\": \"limit_reached\", \"pos\": %.2f}", pos);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_OK;
    }

    xQueueSend(webCommandQueue, &cmd, pdMS_TO_TICKS(10));
    httpd_resp_send(req, "{\"status\": \"ok\"}", -1);
    return ESP_OK;
}

esp_err_t set_mode_handler(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *json = cJSON_Parse(buf);
    if (!json) return ESP_FAIL;

    const char* mode_str = cJSON_GetObjectItem(json, "mode")->valuestring;
    WebCommand cmd;
    cmd.type = WebCommand::CommandType::SET_MODE;
    cmd.modo = (strcmp(mode_str, "web") == 0) ? Exobrazo::ModoControl::WEB : Exobrazo::ModoControl::PRUEBA;
    cJSON_Delete(json);

    set_cors_headers(req);
    xQueueSend(webCommandQueue, &cmd, pdMS_TO_TICKS(10));
    httpd_resp_send(req, "{\"status\": \"ok\"}", -1);
    return ESP_OK;
}

esp_err_t therapy_handler(httpd_req_t *req) {
    WebCommand cmd;
    cmd.type = WebCommand::CommandType::THERAPEUTIC;
    set_cors_headers(req);
    xQueueSend(webCommandQueue, &cmd, pdMS_TO_TICKS(10));
    httpd_resp_send(req, "{\"status\": \"ok\", \"message\": \"Secuencia terapeutica iniciada\"}", -1);
    return ESP_OK;
}

esp_err_t status_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mode", (exobrazo.getModoActual() == Exobrazo::ModoControl::WEB) ? "web" : "prueba");
    
    cJSON *motores = cJSON_CreateArray();
    for (int i = 0; i < 3; i++) {
        cJSON *m = cJSON_CreateObject();
        cJSON_AddNumberToObject(m, "id", i);
        cJSON_AddNumberToObject(m, "posicion", exobrazo.motores[i]->getPosicionActual());
        cJSON_AddItemToArray(motores, m);
    }
    cJSON_AddItemToObject(root, "motores", motores);

    char *json_str = cJSON_PrintUnformatted(root);
    set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free(json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// --- TAREAS ---

void motorPulseTask(void *pvParameters) {
    esp_task_wdt_add(NULL);
    for (;;) {
        for(int i=0; i<3; i++) exobrazo.motores[i]->actualizar();
        esp_task_wdt_reset();
        vTaskDelay(0); 
    }
}

void systemLogicTask(void *pvParameters) {
    WebCommand cmd;
    for (;;) {
        if (xQueueReceive(webCommandQueue, &cmd, 0) == pdTRUE) {
            if (cmd.type == WebCommand::CommandType::MOVE) {
                exobrazo.setModoWeb();
                // PASAMOS EL ÁNGULO: La conversión a pasos se hace dentro de moverMotorWeb
                exobrazo.moverMotorWeb(cmd.motorIndex, cmd.velocidad, cmd.direccion, cmd.angulo);
            } else if (cmd.type == WebCommand::CommandType::SET_MODE) {
                if (cmd.modo == Exobrazo::ModoControl::WEB) exobrazo.setModoWeb();
                else exobrazo.setModoPrueba();
            } else if (cmd.type == WebCommand::CommandType::THERAPEUTIC) {
                exobrazo.setModoWeb();
                exobrazo.ejecutarMovimientoTerapeutico();
            }
        }
        exobrazo.ejecutarCicloPrincipal();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void webServerTask(void *pvParameters) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = 0; 

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_move = { .uri = "/move", .method = HTTP_POST, .handler = move_handler };
        httpd_register_uri_handler(server, &uri_move);
        httpd_uri_t uri_status = { .uri = "/status", .method = HTTP_GET, .handler = status_handler };
        httpd_register_uri_handler(server, &uri_status);
        httpd_uri_t uri_mode = { .uri = "/setMode", .method = HTTP_POST, .handler = set_mode_handler };
        httpd_register_uri_handler(server, &uri_mode);
        httpd_uri_t uri_therapy = { .uri = "/therapy", .method = HTTP_POST, .handler = therapy_handler };
        httpd_register_uri_handler(server, &uri_therapy);

        httpd_uri_t opt_move = { .uri = "/move", .method = HTTP_OPTIONS, .handler = cors_options_handler };
        httpd_register_uri_handler(server, &opt_move);
        httpd_uri_t opt_mode = { .uri = "/setMode", .method = HTTP_OPTIONS, .handler = cors_options_handler };
        httpd_register_uri_handler(server, &opt_mode);
        httpd_uri_t opt_therapy = { .uri = "/therapy", .method = HTTP_OPTIONS, .handler = cors_options_handler };
        httpd_register_uri_handler(server, &opt_therapy);
    }
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase(); nvs_flash_init();
    }
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {};
    const char* ssid = "Exobrazito";
    const char* pass = "embebid2025";
    memcpy(wifi_config.ap.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.ap.password, pass, strlen(pass));
    wifi_config.ap.ssid_len = strlen(ssid);
    wifi_config.ap.channel = 1;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.max_connection = 4;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten((adc1_channel_t)ADC_CHANNEL_7, ADC_ATTEN_DB_11);
    adc1_config_channel_atten((adc1_channel_t)ADC_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten((adc1_channel_t)ADC_CHANNEL_5, ADC_ATTEN_DB_11);

    webCommandQueue = xQueueCreate(10, sizeof(WebCommand));
    exobrazo.iniciar();

    xTaskCreatePinnedToCore(motorPulseTask, "PulseTask", 4096, NULL, 24, NULL, 1);
    xTaskCreatePinnedToCore(systemLogicTask, "LogicTask", 8192, NULL, 10, NULL, 0);
    xTaskCreatePinnedToCore(webServerTask, "WebTask", 10240, NULL, 5, NULL, 0);
}