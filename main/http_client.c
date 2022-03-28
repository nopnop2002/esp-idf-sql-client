/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_http_client.h" 
#include "esp_tls.h" 
#include "cJSON.h"



static const char *TAG = "HTTP";

extern EventGroupHandle_t s_key_event_group;
/* Is the Enter key entered */
extern int KEYBOARD_ENTER_BIT;


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len;		 // Stores number of bytes read
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
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
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, output_len=%d", output_len);
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, content_length=%d", esp_http_client_get_content_length(evt->client));
			// If user_data buffer is configured, copy the response into the buffer
			if (evt->user_data) {
				memcpy(evt->user_data + output_len, evt->data, evt->data_len);
			} else {
				if (output_buffer == NULL && esp_http_client_get_content_length(evt->client) > 0) {
					output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
					output_len = 0;
					if (output_buffer == NULL) {
						ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
						return ESP_FAIL;
					}
				}
				memcpy(output_buffer + output_len, evt->data, evt->data_len);
			}
			output_len += evt->data_len;
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
			int mbedtls_err = 0;
			esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
			if (err != 0) {
				if (output_buffer != NULL) {
					free(output_buffer);
					output_buffer = NULL;
				}
				output_len = 0;
				ESP_LOGE(TAG, "Last esp error code: 0x%x", err);
				ESP_LOGE(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			break;
	}
	return ESP_OK;
}

void JSON_Record(const cJSON * const array) {
		int id = cJSON_GetObjectItem(array,"id")->valueint;
		int user_id = cJSON_GetObjectItem(array,"user_id")->valueint;
		int category_id = cJSON_GetObjectItem(array,"category_id")->valueint;
		char *content = cJSON_GetObjectItem(array,"content")->valuestring;
		ESP_LOGI(TAG, "%d\t%d\t%d\t%s", id, user_id, category_id, content);
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

void JSON_Print(const cJSON * const root) {
	ESP_LOGI(TAG, "-----------------------------------------");
	ESP_LOGD(TAG, "root->type=%s", JSON_Types(root->type));
	if (cJSON_IsArray(root)) {
		ESP_LOGD(TAG, "JSON_Print root->type is Array");
		int root_array_size = cJSON_GetArraySize(root); 
		ESP_LOGD(TAG, "JSON_Print root_array_size=%d", root_array_size);
		for (int i=0;i<root_array_size;i++) {
			cJSON *record = cJSON_GetArrayItem(root,i);
			JSON_Record(record);
		}
	} else {
		ESP_LOGD(TAG, "JSON_Print root->type is Object");
		JSON_Record(root);
	}
	ESP_LOGI(TAG, "-----------------------------------------");
}


void JSON_Analyze(const cJSON * const root) {
	ESP_LOGD(TAG, "root->type=%s", JSON_Types(root->type));
	cJSON *current_element = NULL;
	ESP_LOGD(TAG, "root->child=%p", root->child);
	ESP_LOGD(TAG, "root->next =%p", root->next);
	static char* string;
	cJSON_ArrayForEach(current_element, root) {
		ESP_LOGD(TAG, "type=%s", JSON_Types(current_element->type));
		ESP_LOGD(TAG, "current_element->string=%p", current_element->string);
		if (current_element->string) {
			//const char* string = current_element->string;
			string = current_element->string;
			ESP_LOGI(TAG, "[%s]", string);
		}
		if (cJSON_IsInvalid(current_element)) {
			ESP_LOGW(TAG, "Invalid");
		} else if (cJSON_IsFalse(current_element)) {
			ESP_LOGI(TAG, "[%s] False", string);
		} else if (cJSON_IsTrue(current_element)) {
			ESP_LOGI(TAG, "[%s] True", string);
		} else if (cJSON_IsNull(current_element)) {
			ESP_LOGI(TAG, "[%s] Null", string);
		} else if (cJSON_IsNumber(current_element)) {
			int valueint = current_element->valueint;
			double valuedouble = current_element->valuedouble;
			ESP_LOGI(TAG, "[%s] int=%d double=%f", string, valueint, valuedouble);
		} else if (cJSON_IsString(current_element)) {
			const char* valuestring = current_element->valuestring;
			ESP_LOGI(TAG, "[%s] %s", string, valuestring);
		} else if (cJSON_IsArray(current_element)) {
			ESP_LOGD(TAG, "Array");
			//JSON_Analyze(current_element);
			JSON_Print(current_element);
		} else if (cJSON_IsObject(current_element)) {
			//ESP_LOGI(TAG, "Object");
			JSON_Analyze(current_element);
		} else if (cJSON_IsRaw(current_element)) {
			ESP_LOGW(TAG, "Raw(Not support)");
		}
	}
}

#define MAX_HTTP_OUTPUT_BUFFER 2048

esp_err_t http_client_get(char * path)
{
	ESP_LOGI(TAG, "http_client_get path=[%s]",path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	//http://192.168.10.43:8080/api.php/records/posts
	sprintf(url, "http://%s:%s/%s", CONFIG_ESP_WEB_SERVER_IP, CONFIG_ESP_WEB_SERVER_PORT, CONFIG_ESP_PHP_PATH);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGI(TAG, "url=%s",url);
	
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,			 // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGI(TAG, "\n%s", local_response_buffer);

		ESP_LOGI(TAG, "Deserialize.....");
		cJSON *root = cJSON_Parse(local_response_buffer);
		//JSON_Print(root);
		if (strlen(path)) {
			JSON_Print(root);
		} else {
			JSON_Analyze(root);
		}
		cJSON_Delete(root);
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return err;
}

int http_client_post(void)
{
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	//http://192.168.10.43:8080/api.php/records/posts
	sprintf(url, "http://%s:%s/%s", CONFIG_ESP_WEB_SERVER_IP, CONFIG_ESP_WEB_SERVER_PORT, CONFIG_ESP_PHP_PATH);
	ESP_LOGI(TAG, "url=%s",url);

	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,					 // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	long id = 0;
	const char *post_data = "user_id=1&category_id=3&content=Hello World";
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_post_field(client, post_data, strlen(post_data));
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGI(TAG, "\n%s", local_response_buffer);
		id = strtol(local_response_buffer, NULL, 10);
	} else {
		ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return (int)id;
}

esp_err_t http_client_put(char * path)
{
	ESP_LOGI(TAG, "http_client_put path=%s", path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	//http://192.168.10.43:8080/api.php/records/posts
	sprintf(url, "http://%s:%s/%s", CONFIG_ESP_WEB_SERVER_IP, CONFIG_ESP_WEB_SERVER_PORT, CONFIG_ESP_PHP_PATH);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGD(TAG, "url=%s",url);
	
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,			 // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// PUT
	const char *post_data = "content=Hello Japan";
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_PUT);
	esp_http_client_set_post_field(client, post_data, strlen(post_data));
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGI(TAG, "\n%s", local_response_buffer);
	} else {
		ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return err;
}

esp_err_t http_client_delete(char * path)
{
	ESP_LOGI(TAG, "http_client_delete path=%s", path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	//http://192.168.10.43:8080/api.php/records/posts
	sprintf(url, "http://%s:%s/%s", CONFIG_ESP_WEB_SERVER_IP, CONFIG_ESP_WEB_SERVER_PORT, CONFIG_ESP_PHP_PATH);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGD(TAG, "url=%s",url);

	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,			 // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// DELETE
	//int url_length = strlen(url);
	//if (url[url_length-1] != '/') strcat(url,"/");
	//strcat(url, path);
	//ESP_LOGI(TAG, "url=%s",url);
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_DELETE);
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGI(TAG, "\n%s", local_response_buffer);
	} else {
		ESP_LOGE(TAG, "HTTP DELETE request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return err;
}

void http_client(void *pvParameters)
{
	//Read all data
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Read all data");
	xEventGroupClearBits(s_key_event_group, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(s_key_event_group, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	http_client_get("");

	//Read by id
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Read by id");
	xEventGroupClearBits(s_key_event_group, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(s_key_event_group, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	http_client_get("2");

	//Create new record
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Create new record");
	xEventGroupClearBits(s_key_event_group, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(s_key_event_group, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	int new_id = http_client_post();
	ESP_LOGI(TAG, "new_id=%d", new_id);
	char path[10];
	sprintf(path, "%d", new_id);
	http_client_get(path);

	//Update record
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Update new record");
	xEventGroupClearBits(s_key_event_group, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(s_key_event_group, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	if (http_client_get(path) == ESP_OK) {
		http_client_put(path);
		http_client_get(path);
	}

	//Delete record
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Delete new record");
	xEventGroupClearBits(s_key_event_group, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(s_key_event_group, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	if (http_client_get(path) == ESP_OK) {
		http_client_delete(path);
		http_client_get("");
	}
	ESP_LOGI(TAG, "All Finish!!");

	while(1) {
		vTaskDelay(1);
	}
}
