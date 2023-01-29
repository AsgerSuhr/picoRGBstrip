#
# Project to read temperature from some DS18B20 sensors and publish them to MQTT
# Then Home Assistant can read the temperature from MQTT and use it as a climate
# entity to control the heating system using relays connected to the Pico.
#
# In brief, this project:
#
# 1. Loads the secrets from the secrets.py file and loads the common MLHA library
# in order to connect to WiFi, MQTT and Home Assistant.
# 2. Sets up the DS18B20 sensors and the relays to control the heating system.
# 5. Sets up the MQTT publishing of the configuration of the sensors to Home Assistant.
# 3. Sets up the MQTT subscriptions to receive commands from Home Assistant.
# 4. Sets up the MQTT publishing of the temperature and the status of the relays.
# 6. Sets up a timer to read the temperature from the sensors and publish it to MQTT.
#
# Notes on changing the code:
# - When the program is running, the on-board LED of the Pico board is lit. If 
#   you are using a UI like the recommended Thonny, you can simply halt the 
#   program when LED is on and update the program.
#
#     -- Jesús Ramos, 01-Dec-2022
#

import json
from secrets import wifi_SSID, wifi_password, mqtt_server, mqtt_port, mqtt_user, mqtt_password
from lib.makerlab.mlha import MLHA 
from machine import Pin, Timer
import onewire, ds18x20
import time
import gc
from machine import Pin
from rp2 import PIO, StateMachine, asm_pio
from time import sleep
import math
from APA102_PIO import *

# Pins definition ===================================
led_b = Pin(6, Pin.OUT)
latest = 0
mlha = None # WiFi, MQTT and HomeAssistant library
extracted_data = {
      "brightness": latest,
      "color_mode": "rgb",
      "color_temp": 155,
      "color": {
        "r": 127,
        "g": 127,
        "b": 127,
        "c": 100,
        "w": 50,
        "x": 0.406,
        "y": 0.301,
        "h": 344.0,
        "s": 29.412
      },
      "effect": "colorloop",
      "state": "OFF",
      "transition": 2,
    }
# Functions =========================================

def convertRange( value, r1, r2 ):
    return ( value - r1[ 0 ] ) * ( r2[ 1 ] - r2[ 0 ] ) / ( r1[ 1 ] - r1[ 0 ] ) + r2[ 0 ]

def msg_received(topic, msg, retained, duplicate):
    global latest, extracted_data
    msg = json.loads(msg.decode('utf-8'))
    print(msg)
    if topic == "system/status":
        mlha.publish("system/status", "online")
    elif topic == "light/LED_status":
        if msg == b"True":
            print(f'topic: {topic}')
            print(f'message: {msg}')
            led_b.value(1)
            mlha.publish("state", 'True')
        elif msg == b"False":
            print(f'topic: {topic}')
            print(f'message: {msg}')
            led_b.value(0)
            mlha.publish("state", 'False')
        elif msg['state'] == 'ON':
            extracted_data['state'] = 'ON'
        elif msg['state'] == 'OFF':
            extracted_data['state'] = 'OFF'
        
        if 'brightness' in msg.keys():
            extracted_data['brightness'] = msg['brightness']
        
        if 'color' in msg.keys():
            r, g, b = msg['color']['r'], msg['color']['g'], msg['color']['b']
            extracted_data['color'] = {'r':msg['color']['r'], 'g':msg['color']['g'], 'b':msg['color']['b']}
        
        extracted_data['color_mode'] = 'rgb'
        
    else:
        print("Unknown topic")
    
    time.sleep(1)
    stringified_data = json.dumps(extracted_data)
    print(stringified_data)
    mlha.publish("state", stringified_data)


# Main =============================================
paral_sm = StateMachine(0, paral_prog, freq=5 * 1000 * 1000, sideset_base=Pin(2), out_base=Pin(3))
paral_sm.active(1)

# Initialize main component (WiFi, MQTT and HomeAssistant)
mlha = MLHA(wifi_SSID, wifi_password, mqtt_server, mqtt_port, mqtt_user, mqtt_password, name='LEDstrip')
mlha.set_callback(msg_received)

print("New session being set up")

# Publish config for sensors
print("Publishing config to Homeassistant")
mlha.publish_config("LED_status", "LED Status", "light", None, None, None)
print("Connected to MQTT broker and subscribed to topics")

print("Initialization complete, free memory: " + str(gc.mem_free()))
print("Ready to send/receive data")
mlha.publish("system/status", "online", retain=True)

# Main loop
while True:
    if extracted_data['state'] == 'ON':
        paral_sm.put(0)
        for i in range(30):
            result = put_rgb888(paral_sm, int(convertRange(extracted_data['brightness'], [0,255], [0,15])), convertRange(extracted_data['color']['r'], [0,255], [0,127]), convertRange(extracted_data['color']['g'], [0,255], [0,127]), convertRange(extracted_data['color']['b'], [0,255], [0,127]))
        paral_sm.put(1)
        print(result)
    else:
        paral_sm.put(0)
        for i in range(30):
            put_rgb888(paral_sm, 0, 0, 0, 0)
        paral_sm.put(1)        
    try:
        mlha.check_mqtt_msg()
        time.sleep_ms(500)
    except Exception as ex:
        print("error: " + str(ex))
        machine.reset()
