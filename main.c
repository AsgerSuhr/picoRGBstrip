#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt_priv.h"
#include "tusb.h"
#include "apa102.h"
#include "hardware/watchdog.h"
/* 
include secrets.h file containing ssid and password for local network
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"
#define HA_PASS "MQTT BROKER PASSWORD"
#define HA_IP "HOME ASSISTANT IP ADDRESS"
*/
#include "secrets.h"

#define DEVICE_NAME "LED strip"
#define NICE_NAME "Knight Rider"
#define HA_USERNAME "homeassistant"
#define KEEP_ALIVE 60
#define LWT_TOPIC "LED strip worktable" // Last Will Testament topic
#define LWT_MESS "ON" // Last Will Testament message
#define LWT_QOS 2 // Quality of service 0(worst), 1 or 2(best)
#define LWT_RETAIN 1
#define MODEL "Pico W"
#define MANUFACTURER "Raspberry Pi" 
#define VERSION "0.1"
#define MAX_BRIGHTNESS 31 // Due to APA102 the brightness range is 0-31
#define ID_LEN 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1

char ID[ID_LEN];
uint8_t MAC[6];
// ip_addr_t test;
char IP_ADDRESS[16];
char MAC_STRING[20];

uint8_t wave_table[TABLE_SIZE]; // For the colorwheel effect

// Variables used to store values for the RGB LEDs
bool NEW_DATA = false; // check for new data coming from Home Assistant/MQTT broker
bool STATE = false; // Check whether light is On or Off
uint8_t BRIGHTNESS = MAX_BRIGHTNESS; 
uint8_t R = 255;
uint8_t G = 255;
uint8_t B = 255;
bool COLORWHEEL = false;

bool feed_the_dog = true;

typedef struct MQTT_CLIENT_DATA_T {
	mqtt_client_t *mqtt_client_inst;
	struct mqtt_connect_client_info_t mqtt_client_info;
	uint8_t data[MQTT_OUTPUT_RINGBUF_SIZE];
	uint8_t topic[100];
	uint32_t len;
} MQTT_CLIENT_DATA_T;

MQTT_CLIENT_DATA_T *mqtt;

struct mqtt_connect_client_info_t mqtt_client_info =
{
	DEVICE_NAME,
	HA_USERNAME, /* user */
	HA_PASS, /* pass */
	KEEP_ALIVE,  /* keep alive */
	LWT_TOPIC, /* will_topic */
	LWT_MESS, /* will_msg */
	LWT_QOS,    /* will_qos */
	LWT_RETAIN     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
	, NULL
#endif
};

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
	if(result != ERR_OK) {
		printf("Publish result: %d\n", result);
	}
}

err_t register_publish(mqtt_client_t *client, void *arg)
{
	char pub_payload[MQTT_OUTPUT_RINGBUF_SIZE];
	snprintf(pub_payload, MQTT_OUTPUT_RINGBUF_SIZE, 
		"{"
				"\"state_topic\": \"%s-%s/state\","
				"\"effect_list\": ["
						"\"off\","
						"\"colorwheel\""
				"],"
				"\"unique_id\": \"%s-%s-pico_status\","
				"\"expire_after\": 60,"
				"\"brightness\": true,"
				"\"brightness_scale\":%d,"
				"\"name\": \"%s\","
				"\"device\": {"
						"\"name\": \"%s\","
						"\"identifiers\": \"%s-%s\","
						"\"manufacturer\": \"%s\","
						"\"model\": \"%s\","
						"\"sw_version\": \"%s\","
						"\"connections\": ["
								"["
										"\"ip\","
										"\"%s\""
								"],"
								"["
										"\"mac\","
										"\"%s\""
								"]"
						"]"
				"},"
				"\"command_topic\": \"%s-%s/light/pico_status\","
				"\"color_mode\": true,"
				"\"availability\": ["
						"{"
								"\"payload_not_available\": \"offline\","
								"\"topic\": \"%s-%s/system/status\","
								"\"payload_available\": \"online\""
						"}"
				"],"
				"\"effect\": true,"
				"\"supported_color_modes\": ["
						"\"rgb\""
				"],"
				"\"schema\": \"json\""
		"}", DEVICE_NAME, ID, DEVICE_NAME, ID, MAX_BRIGHTNESS, NICE_NAME, NICE_NAME, DEVICE_NAME, ID, 
			MANUFACTURER, MODEL, VERSION, IP_ADDRESS, MAC_STRING, DEVICE_NAME, ID, DEVICE_NAME, ID);

	err_t err;
	u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 0; /* No don't retain such crappy payload... */
	cyw43_arch_lwip_begin();
	err = mqtt_publish(client, "homeassistant/light/Picow/light_test/config", pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, arg);
	cyw43_arch_lwip_end();
	if(err != ERR_OK) {
		printf("Publish err: %d\n", err);
	}
	return err;
}

err_t publish_availability(mqtt_client_t *client, void *arg)
{
	char* pub_payload = "online";
	char buffer[100];
	snprintf(buffer, 100, "%s-%s/system/status", DEVICE_NAME, ID);
	err_t err;
	u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 0; /* No don't retain such crappy payload... */
	cyw43_arch_lwip_begin();
	err = mqtt_publish(client, (const char*)buffer, pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, arg);
	cyw43_arch_lwip_end();
	if(err != ERR_OK) {
		printf("Publish err: %d\n", err);
	}
	return err;
}

err_t publish_reportState(mqtt_client_t *client, void *arg)
{	
	char* state;
	char* effect;
	if (STATE)
		state = "\"ON\"";
	else
		state = "\"OFF\"";
	
	if (COLORWHEEL) 
		effect = "\"colorwheel\"";
	else 
		effect = "\"off\"";

	char pub_payload[256];
	snprintf(pub_payload, 256, "{\"state\": %s,"
		"\"brightness\": %d,"
		"\"color_mode\": \"rgb\","
		"\"color\": {"
			"\"r\": %d,"
			"\"g\": %d,"
			"\"b\": %d"
		"},"
		"\"effect\": %s"
	"}", state, BRIGHTNESS, R, G, B, effect);
	char buffer[100];
	snprintf(buffer, 100, "%s-%s/state", DEVICE_NAME, ID);
	err_t err;
	u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 0; /* No don't retain such crappy payload... */
	cyw43_arch_lwip_begin();
	err = mqtt_publish(client, (const char*)buffer, pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, arg);
	cyw43_arch_lwip_end();
	if(err != ERR_OK) {
		printf("Publish err: %d\n", err);
	}
	return err;
}


char **parse_json(const char* string, char *seperators, int *count) {
	int len = strlen(string);
	*count = 0;
	
	int i = 0;
	while (i < len) {

		// run past potential leading separators
		while (i < len) {
			if ( strchr(seperators, string[i]) == NULL )
				break; // If we encounter a separator we break
			i++;
		}

		int old_i = i;
		while (i < len) {
			if ( strchr(seperators, string[i]) != NULL )
				break;
			i++;
		}

		if (i > old_i) *count = *count + 1;
	}

	char **strings = malloc(sizeof(char *) * *count);

	i = 0;
	char buffer[256*4] = "";
	int string_index = 0;
	while (i < len) {

		// run past potential leading separators
		while (i < len) {
			if ( strchr(seperators, string[i]) == NULL )
				break; // If we encounter a separator we break
			i++;
		}

		int j = 0;
		while (i < len) {
			if ( strchr(seperators, string[i]) != NULL )
				break;
			buffer[j] = string[i];
			i++;
			j++;
		}

		if (j > 0) {
			buffer[j] = '\0';
			int to_allocate = sizeof(char) * ( strlen(buffer) + 1 );
			strings[string_index] = malloc(to_allocate);
			strcpy(strings[string_index], buffer);
			string_index++;
		}

	}
	
	return strings;
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
	printf("mqtt_incoming_data_cb\n");
	MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
	LWIP_UNUSED_ARG(data);
	strncpy((char*)mqtt_client->data, (char*)data, len);

	mqtt_client->len=len;
	mqtt_client->data[len]='\0';

	char buffer[100];
	snprintf(buffer, 100, "%s-%s/light/pico_status", DEVICE_NAME, ID);

	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

	// Parse the data recieved from the broker
	if (strcmp((const char*)mqtt->topic, (const char*)buffer) == 0) {
		
		// split the json string into pairs of key:item
		int count_strings = 0;
		char **split_strings = parse_json( mqtt->data, " ,{}", &count_strings );
		
		// iterate through the pairs
		for (int i = 0; i < count_strings; i++) {

			// check if "state" substring is in the command
			if ( strstr(split_strings[i], "state") != NULL ) 
			{
				
				// check if "ON" substring is in the command
				if ( strstr(split_strings[i], "ON") != NULL ) {
					STATE = true;
				} else {
					STATE = false;
				}

			} 
			else if ( strstr(split_strings[i], "brightness") != NULL ) 
			{
				char *last = strrchr(split_strings[i], ':');
				if (last != NULL)
				{
					printf("%s\n", last+1);
					BRIGHTNESS = atoi(last+i);
				} 
			} 
			else if ( strstr(split_strings[i], "\"r\"") != NULL ) 
			{
				char *r = strrchr(split_strings[i], ':');
				if (r != NULL)
				{
					printf("%s\n", r+1);
					R = atoi(r+1);
				} 
			}
			else if ( strstr(split_strings[i], "\"g\"") != NULL ) 
			{
				char *g = strrchr(split_strings[i], ':');
				if (g != NULL)
				{
					printf("%s\n", g+1);
					G = atoi(g+1);
				} 
			}
			else if ( strstr(split_strings[i], "\"b\"") != NULL ) 
			{
				char *b = strrchr(split_strings[i], ':');
				if (b != NULL)
				{
					printf("%s\n", b+1);
					B = atoi(b+1);
				} 
			}
			else if ( strstr(split_strings[i], "\"effect\"") != NULL ) 
			{
				char *effect = strrchr(split_strings[i], ':');
				if (effect != NULL)
				{
					printf("%s\n", effect+1);
					if ( strcmp(effect+1, "\"colorwheel\"") == 0 )
					{
						COLORWHEEL = true;
					} else if ( strcmp(effect+1, "\"off\"") == 0 )
					{
						COLORWHEEL = false;
					}
				} 
			}

			printf("%s\n", split_strings[i]);

		}

		// free the dynamically allocated space for each string
		for (int i = 0; i < count_strings; i++)
			free(split_strings[i]);
		
		// free the dynamically allocated space for the array of pointers to strings
		free(split_strings);

	}

	publish_reportState(mqtt->mqtt_client_inst, mqtt);
	NEW_DATA = true;
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

}


static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
	MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
	strcpy((char*)mqtt_client->topic, (char*)topic);

}

static void mqtt_request_cb(void *arg, err_t err) {
	MQTT_CLIENT_DATA_T* mqtt_client = ( MQTT_CLIENT_DATA_T*)arg;

	LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n", mqtt_client->mqtt_client_info.client_id, (int)err));
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
	MQTT_CLIENT_DATA_T* mqtt_client = (MQTT_CLIENT_DATA_T*)arg;
	LWIP_UNUSED_ARG(client);

	LWIP_PLATFORM_DIAG(("MQTT client \"%s\" connection cb: status %d\n", mqtt_client->mqtt_client_info.client_id, (int)status));

	char buffer[100];
	snprintf(buffer, 100, "%s-%s/light/pico_status", DEVICE_NAME, ID);

	if (status == MQTT_CONNECT_ACCEPTED) {
		printf("MQTT_CONNECT_ACCEPTED\n");

		mqtt_sub_unsub(client,
						(const char*)buffer, 2,
						mqtt_request_cb, arg,
						1);    
	} else {
		feed_the_dog = false;
	}
}

int main()
{
	stdio_init_all();

    // if (watchdog_caused_reboot()) {
    //     printf("Rebooted by Watchdog!\n");
    //     return 0;
    // } else {
    //     printf("Clean boot\n");
    // }
	
	pico_get_unique_board_id_string(ID, ID_LEN);

	mqtt=(MQTT_CLIENT_DATA_T*)calloc(1, sizeof(MQTT_CLIENT_DATA_T));
	
	if (!mqtt) {
			printf("mqtt client instant ini error\n");
			return 0;
	}

	mqtt->mqtt_client_info = mqtt_client_info;
	if (cyw43_arch_init())
	{
			printf("failed to initialise\n");
			return 1;
	}

	cyw43_arch_enable_sta_mode();
	if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
	{
			printf("failed to connect\n");
			return 1;
	}

	ip_addr_t addr;
	if (!ip4addr_aton(HA_IP, &addr)) {
			printf("ip error\n");
			return 0;
	}

	
	mqtt->mqtt_client_inst = mqtt_client_new();
	mqtt_set_inpub_callback(mqtt->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, mqtt);

	err_t err = mqtt_client_connect(mqtt->mqtt_client_inst, &addr, MQTT_PORT, &mqtt_connection_cb, mqtt, &mqtt->mqtt_client_info);
	if (err != ERR_OK) {
		printf("connect error\n");
		return 0;
	}

	sleep_ms(1000); // wait for the connection to stabilise

	int success = cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, MAC);

	uint32_t ip_binary = cyw43_state.netif[0].ip_addr.addr;
    uint8_t CP1 = (ip_binary & 0xff000000UL) >> 24;
    uint8_t CP2 = (ip_binary & 0x00ff0000UL) >> 16;
    uint8_t CP3 = (ip_binary & 0x0000ff00UL) >>  8;
    uint8_t CP4 = (ip_binary & 0x000000ffUL)      ;
    snprintf(IP_ADDRESS, 16, "%d.%d.%d.%d", CP4,CP3,CP2,CP1);
	snprintf(MAC_STRING, 20, "%02x:%02x:%02x:%02x:%02x:%02x", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
    printf("IP address: %s\n", IP_ADDRESS);
	printf("MAC: %s", MAC_STRING);

	for (int i=0; i<5; i++)
	{
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
		sleep_ms(250);
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
		sleep_ms(250);
	}

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &apa102_mini_program);
    apa102_mini_program_init(pio, sm, offset, SERIAL_FREQ, PIN_CLK, PIN_DIN);

    for (int i = 0; i < TABLE_SIZE; ++i)
        wave_table[i] = powf(sinf(i * M_PI / TABLE_SIZE), 5.f) * 255;
	uint t = 0;

	register_publish(mqtt->mqtt_client_inst, mqtt);
	sleep_ms(1000);

	publish_availability(mqtt->mqtt_client_inst, mqtt);
	publish_reportState(mqtt->mqtt_client_inst, mqtt);

	watchdog_enable(10000, 1);

	int secondsPassed = 0;
	while(1) {
		secondsPassed++;
		if (feed_the_dog) watchdog_update();
		sleep_ms(1);

		while (COLORWHEEL && STATE) {
			put_start_frame(pio, sm);
			for (int i = 0; i < N_LEDS; ++i) {
				put_rgb888(pio, sm,
						wave_table[(i + t) % TABLE_SIZE],
						wave_table[(2 * i + 3 * 2) % TABLE_SIZE],
						wave_table[(3 * i + 4 * t) % TABLE_SIZE],
						BRIGHTNESS
				);
			}
			put_end_frame(pio, sm);
			sleep_ms(25);
			++t;
			if (feed_the_dog) watchdog_update();
		}

		if (NEW_DATA) 
		{
			if (STATE)
			{
				put_start_frame(pio, sm);
				for (int i = 0; i < N_LEDS; ++i) {
					put_rgb888(pio, sm, R, G, B, BRIGHTNESS);
				};
				put_end_frame(pio, sm);
			}
			else 
			{
				put_start_frame(pio, sm);
				for (int i = 0; i < N_LEDS; ++i) {
					put_rgb888(pio, sm, 0, 0, 0, 0);
				};
				put_end_frame(pio, sm);
			}

			NEW_DATA = false;
		}

		if (secondsPassed == 60*1000) 
		{
			secondsPassed = 0;
			publish_availability(mqtt->mqtt_client_inst, mqtt);
			printf("Sending availability...\n");
		}


	}
	
	
	return 0;
}