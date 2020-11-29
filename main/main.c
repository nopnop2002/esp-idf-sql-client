/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define ESP_WIFI_SSID "mywifissid"
*/
#define ESP_WIFI_SSID			CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS			CONFIG_ESP_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY		CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event */
/* - Are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;


static const char *TAG = "MAIN";

static int s_retry_num = 0;

EventGroupHandle_t xEventGroup;


static void event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < ESP_MAXIMUM_RETRY) {
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

#if 0
	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_create_default());
#endif
	ESP_ERROR_CHECK(esp_netif_init());
	
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = ESP_WIFI_SSID,
			.password = ESP_WIFI_PASS
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s", ESP_WIFI_SSID);
}


/* Is the Enter key entered */
int KEYBOARD_ENTER_BIT = BIT2;

void keyin_task(void *pvParameters)
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

void http_client(void *pvParameters);

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

	//Create EventGroup
	xEventGroup = xEventGroupCreate();

	//Check everything was created
	configASSERT( xEventGroup );

	//Start keyboard task
	xTaskCreate(keyin_task, "KEYIN", 1024*2, NULL, 2, NULL);

	//Start http client task
	xTaskCreate(http_client, "HTTP", 1024*8, NULL, 2, NULL);
}
