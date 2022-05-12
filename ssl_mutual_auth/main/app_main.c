/* MQTT Mutual Authentication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "esp_sleep.h"

#include "cJSON.h"

#define BROKER_USERNAME "esp32"
#define BROKER_PASSWORD "Password01"

static const char *TAG = "MQTTS_EXAMPLE";

esp_mqtt_client_handle_t client;

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");
extern const uint8_t server_cert_pem_start[] asm("_binary_mosquitto_org_crt_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_mosquitto_org_crt_end");

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    bool stan;
} RGB_LED_str;

typedef struct
{
    bool isChange;
    RGB_LED_str RGB_LED;
    uint8_t OC_W;
    uint8_t OC_E;
    bool scrt;
    bool light;
} set_device_str;

typedef struct 
{
    uint16_t temp;
    bool scrt_stan;

}get_device_str;

char get_buf[10]={"/set"};

static set_device_str set_str;
static get_device_str get_str;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}


static bool ParseJSON_set(char *buf)
{
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL)
    {
        ESP_LOGW(TAG, "errorPars");
        return 0;
    }
    
    if(cJSON_HasObjectItem(root, "status"))
    {
        if (!cJSON_GetObjectItem(root, "status")->valueint)
        {
            ESP_LOGW(TAG, "Error Parsing status");
            return 0;
        }
    }
    else
    {    
        ESP_LOGW(TAG, "Error Parsing status, the object does not exist");
        return 0;
    }

    if(cJSON_HasObjectItem(root, "LED"))
    {
        cJSON *LED = cJSON_GetObjectItem(root, "LED");
            
        if(cJSON_HasObjectItem(LED, "R"))
            set_str.RGB_LED.R = cJSON_GetObjectItem(LED, "R")->valueint;
        else
        {    
            ESP_LOGW(TAG, "Error Parsing LED->R, the object does not exist");
            return 0;
        } 
        
        if(cJSON_HasObjectItem(LED, "G"))
            set_str.RGB_LED.G = cJSON_GetObjectItem(LED, "G")->valueint;
        else
        {    
            ESP_LOGW(TAG, "Error Parsing LED->G, the object does not exist");
            return 0;
        }

        if(cJSON_HasObjectItem(LED, "B"))
            set_str.RGB_LED.B = cJSON_GetObjectItem(LED, "B")->valueint;
        else
        {    
            ESP_LOGW(TAG, "Error Parsing LED->G, the object does not exist");
            return 0;
        }    
        
        if(cJSON_HasObjectItem(LED, "stan"))
            set_str.RGB_LED.stan = cJSON_GetObjectItem(LED, "stan")->valueint;
        else
        {    
            ESP_LOGW(TAG, "Error Parsing LED->stan, the object does not exist");
            return 0;
        }
    }  
    else
    {    
        ESP_LOGW(TAG, "Error Parsing LED, the object does not exist");
        return 0;
    } 
    
    

    if(cJSON_HasObjectItem(root, "OC_W"))
        set_str.OC_W = cJSON_GetObjectItem(root, "OC_W")->valueint;
    else
    {    
        ESP_LOGW(TAG, "Error Parsing OC_W, the object does not exist");
        return 0;
    } 
    
    if(cJSON_HasObjectItem(root, "OC_E"))
        set_str.OC_E = cJSON_GetObjectItem(root, "OC_E")->valueint;
    else
    {    
        ESP_LOGW(TAG, "Error Parsing OC_E, the object does not exist");
        return 0;
    } 

    if(cJSON_HasObjectItem(root, "scrt"))
        set_str.scrt = cJSON_GetObjectItem(root, "scrt")->valueint;
    else
    {    
        ESP_LOGW(TAG, "Error Parsing scrt, the object does not exist");
        return 0;
    }

     
    if(cJSON_HasObjectItem(root, "light"))
        set_str.light = cJSON_GetObjectItem(root, "light")->valueint;
    else
    {    
        ESP_LOGW(TAG, "Error Parsing light, the object does not exist");
        return 0;
    }

    set_str.isChange = true;

    cJSON_Delete(root);

    ESP_LOGW(TAG, "LED:R=%d,G=%d,B=%d, stan=%d\n", set_str.RGB_LED.R, set_str.RGB_LED.G, set_str.RGB_LED.B, set_str.RGB_LED.stan);
    ESP_LOGW(TAG, "OC: E=%d, W=%d\n", set_str.OC_E, set_str.OC_W);
    ESP_LOGW(TAG, "scrt=%d\n", set_str.scrt);
    ESP_LOGW(TAG, "light=%d\n", set_str.light);

    return 1;
}

static bool BuilderJSON_get(char *buf, int *len_buf)
{
    cJSON *root, *LED;
    root=cJSON_CreateObject();
    cJSON_AddTrueToObject (root, "status");

    cJSON_AddItemToObject(root, "LED", LED=cJSON_CreateObject());
    cJSON_AddNumberToObject(LED,"R", set_str.RGB_LED.R);
    cJSON_AddNumberToObject(LED,"G", set_str.RGB_LED.G);
    cJSON_AddNumberToObject(LED,"B", set_str.RGB_LED.B);
    
    if(set_str.RGB_LED.stan)
        cJSON_AddTrueToObject (LED, "status");
    else cJSON_AddFalseToObject (LED, "status");

    if(set_str.RGB_LED.stan)
        cJSON_AddTrueToObject (root, "OC_W");
    else cJSON_AddFalseToObject (root, "OC_W");

    if(set_str.RGB_LED.stan)
        cJSON_AddTrueToObject (root, "OC_E");
    else cJSON_AddFalseToObject (root, "OC_E");

    if(set_str.RGB_LED.stan)
        cJSON_AddTrueToObject (root, "scrt");
    else cJSON_AddFalseToObject (root, "scrt");

    if(set_str.RGB_LED.stan)
        cJSON_AddTrueToObject (root, "light");
    else cJSON_AddFalseToObject (root, "light");

    
    cJSON_AddNumberToObject(root, "temp", get_str.temp);
    
    if(set_str.RGB_LED.stan)
        cJSON_AddTrueToObject (root, "scrtStan");
    else cJSON_AddFalseToObject (root, "scrtStan");

    char *buffer=cJSON_PrintUnformatted(root);
    // ESP_LOGW(TAG, "len=%s", buffer );
    *len_buf=strlen(buffer);
    if(*len_buf>134)
     ESP_LOGW(TAG, "len=%d", *len_buf );
    memcpy(buf, buffer, *len_buf);
    cJSON_Delete(root); 
    
    return 1;
} 


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{   
    char buf_topic[10]={0};
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/set", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/set", "SUBSCRIBED_ESP32", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        esp_wifi_stop();
        esp_deep_sleep(3000000);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        memcpy(buf_topic,event->topic,event->topic_len);
        
        if(strcmp(buf_topic, get_buf)==0)
            ParseJSON_set(event->data);
        else ESP_LOGI(TAG, "error topic=%s, len=%d",buf_topic, event->topic_len );



        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtts://764d530e0f154b428da024a2b016fa03.s2.eu.hivemq.cloud",
        .client_cert_pem = (const char *)client_cert_pem_start,
        .client_key_pem = (const char *)client_key_pem_start,
        .cert_pem = (const char *)server_cert_pem_start,
        .port = 8883,
        .username = BROKER_USERNAME,
        .password = BROKER_PASSWORD,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void publish_task()
{   char buf[200]={0};
    int lenn=0;
    while (1)
    {   
        memset(buf, 0, 200 );
        BuilderJSON_get(buf, &lenn);
        ESP_LOGI(TAG, "lenn=%d",lenn );
        esp_mqtt_client_publish(client, "/get", buf, lenn, 0, 0);
        vTaskDelay(200);
        esp_wifi_stop();
        esp_deep_sleep(10000000);
    }
    
}


void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();

    xTaskCreate(publish_task, "publish_task", 3000,0,2,NULL);
}
