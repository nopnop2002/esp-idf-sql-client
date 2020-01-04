/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_http_client.h" 
#include "esp_tls.h" 
#include "cJSON.h"


/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID			CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS			CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY		CONFIG_ESP_MAXIMUM_RETRY
#define EXAMPLE_ESP_MYSQL_SERVER_IP		CONFIG_ESP_MYSQL_SERVER_IP
#define EXAMPLE_ESP_MYSQL_SERVER_PORT	CONFIG_ESP_MYSQL_SERVER_PORT
#define EXAMPLE_ESP_PHP_PATH			CONFIG_ESP_PHP_PATH

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event */
/* - Are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;


static const char *TAG = "JSON";

static int s_retry_num = 0;

RingbufHandle_t xRingbuffer;

EventGroupHandle_t xEventGroup;
/* - Is the Enter key entered? */
const int KEYBOARD_ENTER_BIT = BIT2;


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			if (!esp_http_client_is_chunked_response(evt->client)) {
				//char buffer[512];
				char *buffer = malloc(evt->data_len + 1);
				esp_http_client_read(evt->client, buffer, evt->data_len);
				buffer[evt->data_len] = 0;
				//ESP_LOGI(TAG, "buffer=%s", buffer);
				UBaseType_t res = xRingbufferSend(xRingbuffer, buffer, evt->data_len, pdMS_TO_TICKS(1000));
				if (res != pdTRUE) {
					ESP_LOGW(TAG, "Failed to xRingbufferSend");
				}
				free(buffer);
			}
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
			break;
	}
	return ESP_OK;
}

static void event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}
		ESP_LOGE(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		//ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta()
{
	wifi_event_group = xEventGroupCreate();

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = EXAMPLE_ESP_WIFI_SSID,
			.password = EXAMPLE_ESP_WIFI_PASS
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s", EXAMPLE_ESP_WIFI_SSID);
}


char *JSON_Types(int type) {
	if (type == cJSON_Invalid) return ("cJSON_Invalid");
	if (type == cJSON_False) return ("cJSON_False");
	if (type == cJSON_True) return ("cJSON_True");
	if (type == cJSON_NULL) return ("cJSON_NULL");
	if (type == cJSON_Number) return ("cJSON_Number");
	if (type == cJSON_String) return ("cJSON_String");
	if (type == cJSON_Array) return ("cJSON_Array");
	if (type == cJSON_Object) return ("cJSON_Object");
	if (type == cJSON_Raw) return ("cJSON_Raw");
	return NULL;
}

void JSON_Parse(const cJSON * const root) {
	//ESP_LOGI(TAG, "root->type=%s", JSON_Types(root->type));
	cJSON *current_element = NULL;
	//ESP_LOGI(TAG, "roo->child=%p", root->child);
	//ESP_LOGI(TAG, "roo->next =%p", root->next);
	cJSON_ArrayForEach(current_element, root) {
		//ESP_LOGI(TAG, "type=%s", JSON_Types(current_element->type));
		//ESP_LOGI(TAG, "current_element->string=%p", current_element->string);
		if (current_element->string) {
			const char* string = current_element->string;
			ESP_LOGI(TAG, "[%s]", string);
		}
		if (cJSON_IsInvalid(current_element)) {
			ESP_LOGI(TAG, "Invalid");
		} else if (cJSON_IsFalse(current_element)) {
			ESP_LOGI(TAG, "False");
		} else if (cJSON_IsTrue(current_element)) {
			ESP_LOGI(TAG, "True");
		} else if (cJSON_IsNull(current_element)) {
			ESP_LOGI(TAG, "Null");
		} else if (cJSON_IsNumber(current_element)) {
			int valueint = current_element->valueint;
			double valuedouble = current_element->valuedouble;
			ESP_LOGI(TAG, "int=%d double=%f", valueint, valuedouble);
		} else if (cJSON_IsString(current_element)) {
			const char* valuestring = current_element->valuestring;
			ESP_LOGI(TAG, "%s", valuestring);
		} else if (cJSON_IsArray(current_element)) {
			//ESP_LOGI(TAG, "Array");
			JSON_Parse(current_element);
		} else if (cJSON_IsObject(current_element)) {
			//ESP_LOGI(TAG, "Object");
			JSON_Parse(current_element);
		} else if (cJSON_IsRaw(current_element)) {
			ESP_LOGI(TAG, "Raw(Not support)");
		}
	}
}

esp_err_t http_client_get(char * path)
{
	ESP_LOGI(TAG, "path=%s",path);
	char url[64];
	//http://192.168.10.43:8080/api.php/records/posts
	sprintf(url, "http://%s:%s/%s", EXAMPLE_ESP_MYSQL_SERVER_IP, EXAMPLE_ESP_MYSQL_SERVER_PORT, EXAMPLE_ESP_PHP_PATH);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGI(TAG, "url=%s",url);
	
	esp_http_client_config_t config = {
		//.url = "http://192.168.10.43:3000/todos",
		.url = url,
		.event_handler = _http_event_handler,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t ret;
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		//Receive an item from no-split ring buffer
		int bufferSize = esp_http_client_get_content_length(client);
		char *buffer = malloc(bufferSize + 1); 
		size_t item_size;
		int	index = 0;
		while (1) {
			char *item = (char *)xRingbufferReceive(xRingbuffer, &item_size, pdMS_TO_TICKS(1000));
			if (item != NULL) {
				for (int i = 0; i < item_size; i++) {
					//printf("%c", item[i]);
					buffer[index] = item[i];
					index++;
					buffer[index] = 0;
				}
				//printf("\n");
				//Return Item
				vRingbufferReturnItem(xRingbuffer, (void *)item);
			} else {
				//Failed to receive item
				ESP_LOGD(TAG, "End of receive item");
				break;
			}
		}
		ESP_LOGI(TAG, "buffer=\n%s", buffer);

		ESP_LOGI(TAG, "Deserialize.....");
		cJSON *root = cJSON_Parse(buffer);
		JSON_Parse(root);
		cJSON_Delete(root);
		free(buffer);
		ret = ESP_OK;

	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		ret = ESP_FAIL;
	}
	esp_http_client_cleanup(client);
	return ret;
}

int http_client_post(void)
{
    char url[64];
    //http://192.168.10.43:8080/api.php/records/posts
    sprintf(url, "http://%s:%s/%s", EXAMPLE_ESP_MYSQL_SERVER_IP, EXAMPLE_ESP_MYSQL_SERVER_PORT, EXAMPLE_ESP_PHP_PATH);
    ESP_LOGI(TAG, "url=%s",url);

    esp_http_client_config_t config = {
        //.url = "http://192.168.10.43:3000/todos",
        .url = url,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // POST
	long id = 0;
	//const char *post_data = "field1=value1&field2=value2";
	const char *post_data = "user_id=1&category_id=3&content=Hello World";
	//esp_http_client_set_url(client, "http://httpbin.org/post");
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        //Receive an item from no-split ring buffer
        int bufferSize = esp_http_client_get_content_length(client);
        char *buffer = malloc(bufferSize + 1);
        size_t item_size;
        int index = 0;
        while (1) {
            char *item = (char *)xRingbufferReceive(xRingbuffer, &item_size, pdMS_TO_TICKS(1000));
            if (item != NULL) {
                for (int i = 0; i < item_size; i++) {
                    //printf("%c", item[i]);
                    buffer[index] = item[i];
                    index++;
                    buffer[index] = 0;
                }
                //printf("\n");
                //Return Item
                vRingbufferReturnItem(xRingbuffer, (void *)item);
            } else {
                //Failed to receive item
                ESP_LOGD(TAG, "End of receive item");
                break;
            }
        }
        ESP_LOGI(TAG, "buffer=\n%s", buffer);
		id = strtol(buffer, NULL, 10);
        free(buffer);

    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
	return (int)id;
}

esp_err_t http_client_put(char * path)
{
	ESP_LOGI(TAG, "path=%s", path);
	char url[64];
	//http://192.168.10.43:8080/api.php/records/posts
	sprintf(url, "http://%s:%s/%s", EXAMPLE_ESP_MYSQL_SERVER_IP, EXAMPLE_ESP_MYSQL_SERVER_PORT, EXAMPLE_ESP_PHP_PATH);
	ESP_LOGD(TAG, "url=%s",url);
	
    esp_http_client_config_t config = {
        //.url = "http://192.168.10.43:3000/todos",
        .url = url,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // PUT
	esp_err_t ret;
	int url_length = strlen(url);
	if (url[url_length-1] != '/') strcat(url,"/");
	strcat(url, path);
	ESP_LOGI(TAG, "url=%s",url);
    const char *post_data = "content=Hello Japan";
    esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_PUT);
	esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        //Receive an item from no-split ring buffer
        int bufferSize = esp_http_client_get_content_length(client);
        char *buffer = malloc(bufferSize + 1);
        size_t item_size;
        int index = 0;
        while (1) {
            char *item = (char *)xRingbufferReceive(xRingbuffer, &item_size, pdMS_TO_TICKS(1000));
            if (item != NULL) {
                for (int i = 0; i < item_size; i++) {
                    //printf("%c", item[i]);
                    buffer[index] = item[i];
                    index++;
                    buffer[index] = 0;
                }
                //printf("\n");
                //Return Item
                vRingbufferReturnItem(xRingbuffer, (void *)item);
            } else {
                //Failed to receive item
                ESP_LOGD(TAG, "End of receive item");
                break;
            }
        }
        ESP_LOGI(TAG, "buffer=\n%s", buffer);
        ret = ESP_OK;
        free(buffer);

    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		ret = ESP_FAIL;
    }
    esp_http_client_cleanup(client);
    return ret;
}

esp_err_t http_client_delete(char * path)
{
    ESP_LOGI(TAG, "path=%s", path);
    char url[64];
    //http://192.168.10.43:8080/api.php/records/posts
    sprintf(url, "http://%s:%s/%s", EXAMPLE_ESP_MYSQL_SERVER_IP, EXAMPLE_ESP_MYSQL_SERVER_PORT, EXAMPLE_ESP_PHP_PATH);
    ESP_LOGD(TAG, "url=%s",url);

    esp_http_client_config_t config = {
        //.url = "http://192.168.10.43:3000/todos",
        .url = url,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // DELETE
    esp_err_t ret;
    int url_length = strlen(url);
    if (url[url_length-1] != '/') strcat(url,"/");
    strcat(url, path);
    ESP_LOGI(TAG, "url=%s",url);
    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_DELETE);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        //Receive an item from no-split ring buffer
        int bufferSize = esp_http_client_get_content_length(client);
        char *buffer = malloc(bufferSize + 1);
        size_t item_size;
        int index = 0;
        while (1) {
            char *item = (char *)xRingbufferReceive(xRingbuffer, &item_size, pdMS_TO_TICKS(1000));
            if (item != NULL) {
                for (int i = 0; i < item_size; i++) {
                    //printf("%c", item[i]);
                    buffer[index] = item[i];
                    index++;
                    buffer[index] = 0;
                }
                //printf("\n");
                //Return Item
                vRingbufferReturnItem(xRingbuffer, (void *)item);
            } else {
                //Failed to receive item
                ESP_LOGD(TAG, "End of receive item");
                break;
            }
        }
        ESP_LOGI(TAG, "buffer=\n%s", buffer);
        ret = ESP_OK;
        free(buffer);

    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        ret = ESP_FAIL;
    }
    esp_http_client_cleanup(client);
    return ret;
}

void keyin(void *pvParameters)
{
	//ESP_LOGI(pcTaskGetTaskName(0), "Start");
	uint16_t c;
	while (1) {
		c = fgetc(stdin);
		if (c == 0xffff) {
			vTaskDelay(10);
			continue;
		}
		//ESP_LOGI(pcTaskGetTaskName(0), "c=%x", c);
		if (c == 0x0a) {
			ESP_LOGD(pcTaskGetTaskName(0), "Push Enter");
			xEventGroupSetBits(xEventGroup, KEYBOARD_ENTER_BIT);
		}
	}
}

void app_main()
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	wifi_init_sta();

	//Create Ring Buffer 
	//No Split
	xRingbuffer = xRingbufferCreate(1024*10, RINGBUF_TYPE_NOSPLIT);
	//Allow_Split
	//xRingbuffer = xRingbufferCreate(1024*10, RINGBUF_TYPE_ALLOWSPLIT);

	//Create EventGroup
	xEventGroup = xEventGroupCreate();

	//Check everything was created
	configASSERT( xRingbuffer );
	configASSERT( xEventGroup );

	//Start keyboard task
	xTaskCreate(keyin, "KEYIN", 1024*2, NULL, 2, NULL);

	//List
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to List");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	http_client_get(""); 

	//Read
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Read");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	http_client_get("1"); 

	//Create
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Create");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	int new_id = http_client_post(); 
	ESP_LOGI(TAG, "new_id=%d", new_id);

	//Update
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Update");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	char path[10];
	sprintf(path, "%d", new_id);
	ret = http_client_get(path); 
	if (ret == ESP_OK) {
		http_client_put(path); 
		http_client_get(path); 
	}

	//Delete
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Delete");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	ret = http_client_get(path); 
	if (ret == ESP_OK) {
		http_client_delete(path); 
		http_client_get(path); 
	}
}
