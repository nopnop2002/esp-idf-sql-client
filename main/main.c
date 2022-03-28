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
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
EventGroupHandle_t s_key_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT  BIT1

static const char *TAG = "MAIN";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
																int32_t event_id, void* event_data)
{
		if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
				esp_wifi_connect();
		} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
				if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
						esp_wifi_connect();
						s_retry_num++;
						ESP_LOGI(TAG, "retry to connect to the AP");
				} else {
						xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
				}
				ESP_LOGI(TAG,"connect to the AP fail");
		} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
				ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
				ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
				s_retry_num = 0;
				xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		}
}

void wifi_init_sta(void)
{
		s_wifi_event_group = xEventGroupCreate();

		ESP_ERROR_CHECK(esp_netif_init());

		ESP_ERROR_CHECK(esp_event_loop_create_default());
		esp_netif_create_default_wifi_sta();

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));

		esp_event_handler_instance_t instance_any_id;
		esp_event_handler_instance_t instance_got_ip;
		ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
																												ESP_EVENT_ANY_ID,
																												&event_handler,
																												NULL,
																												&instance_any_id));
		ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
																												IP_EVENT_STA_GOT_IP,
																												&event_handler,
																												NULL,
																												&instance_got_ip));

		wifi_config_t wifi_config = {
				.sta = {
						.ssid = EXAMPLE_ESP_WIFI_SSID,
						.password = EXAMPLE_ESP_WIFI_PASS,
						/* Setting a password implies station will connect to all security modes including WEP/WPA.
						 * However these modes are deprecated and not advisable to be used. Incase your Access point
						 * doesn't support WPA2, these mode can be enabled by commenting below line */
			 .threshold.authmode = WIFI_AUTH_WPA2_PSK,

						.pmf_cfg = {
								.capable = true,
								.required = false
						},
				},
		};
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
		ESP_ERROR_CHECK(esp_wifi_start() );

		ESP_LOGI(TAG, "wifi_init_sta finished.");

		/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
		 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
		EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
						WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
						pdFALSE,
						pdFALSE,
						portMAX_DELAY);

		/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
		 * happened. */
		if (bits & WIFI_CONNECTED_BIT) {
				ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
								 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
		} else if (bits & WIFI_FAIL_BIT) {
				ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
								 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
		} else {
				ESP_LOGE(TAG, "UNEXPECTED EVENT");
		}

		/* The event will not be processed after unregister */
		ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
		ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
		vEventGroupDelete(s_wifi_event_group);
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
			xEventGroupSetBits(s_key_event_group, KEYBOARD_ENTER_BIT);
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
	s_key_event_group = xEventGroupCreate();
	configASSERT( s_key_event_group );

	//Start keyboard task
	xTaskCreate(keyin_task, "KEYIN", 1024*2, NULL, 2, NULL);

	//Start http client task
	xTaskCreate(http_client, "HTTP", 1024*8, NULL, 2, NULL);
}
