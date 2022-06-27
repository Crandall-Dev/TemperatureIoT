#!/usr/bin/env python3
#
#   IoT system MQTT listener and InfluxDB logger
#
#   Currently, quite simple for initial data collection
#
#   @author Aaron S. Crandall <crandall@gonzaga.edu>
#   @copyright 2022
#

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient
from time import sleep
import json
import logging

global influx_client
global message_count


# ** Create a InfluxDB data point from a temperature message **
def createTemperaturePoint(dat):
    point = {
        "measurement": "temperature",
        "tags": {
            "deviceLocation": dat["location"],
        },
        "fields": {
            "value": dat["value"]
        }
    }
    return point

# ** Create a InfluxDB data point from a humidity message **
def createHumidityPoint(dat):
    point = {
        "measurement": "humidity",
        "tags": {
            "deviceLocation": dat["location"],
        },
        "fields": {
            "value": dat["value"]
        }
    }
    return point



# ** ********************************************************
def on_message(client, userdata, message):
    global message_count
    msg_decoded = str(message.payload.decode("utf-8"))

    print(msg_decoded)

    fields = msg_decoded.split(":")
    dat = {
        "deviceID": fields[0],
        "location": fields[1],
        "measurement": fields[2],
        "value": fields[3]
    }

    newPoint = None
    if dat["measurement"] == "temperature":
        newPoint = createTemperaturePoint(dat)
    elif dat["measurement"] == "humidity":
        newPoint = createHumidityPoint(dat)

    if(newPoint):
        influx_client.write_points([newPoint])

def on_connect(client, userdata, flags, rc):
    logging.info("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("IoT/rawevents")

if __name__ == "__main__":
    formatter = "[%(asctime)s] %(name)s {%(filename)s:%(lineno)d} %(levelname)s - %(message)s"
    logging.basicConfig(level=logging.INFO, format=formatter)
    logging.info("Starting logger.")

    influx_client = InfluxDBClient(host='localhost', port=8086, database='IoT')
    logging.info("Connected to InfluxDB")

    # client.write_points(points)

    mqtt_client = mqtt.Client("pyLogger")
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.enable_logger()

    #mqtt_client.connect("localhost")
    mqtt_client.connect("broker.local")

    try:
        logging.info("Connected to MQTT - starting processing")
        mqtt_client.loop_forever()
    except KeyboardInterrupt:
        print("")
        logging.info("Received interrupt - quitting")
    finally:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()
        influx_client.close()
        logging.info("Disconnected from MQTT & InfluxDB")
    logging.info("Logger terminated.")
