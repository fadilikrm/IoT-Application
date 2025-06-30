import paho.mqtt.client as mqtt
import json
import time
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
from collections import defaultdict

MQTT_BROKER = "10.2.22.192"
MQTT_PORT = 1883
INFLUXDB_URL = "http://10.2.22.192:8086/"
INFLUXDB_TOKEN = "y5rUZo5y2u0pl6NE8OtPCYD-N8BK4KRF5fQtYjokYjIYh_9DP4dQUoKDpTGcft4bbD7IvHaeOaiSldeuBwvgtw=="
INFLUXDB_ORG = "klp2"
INFLUXDB_BUCKET = "IoT"

TOPIC_MEASUREMENT_PAIRS = {
    "fog/iot/agri": "Agriculture",
    "fog/iot/healthcare": "HealthCare",
    "fog/iot/city": "City",
    "fog/iot/home": "Home",
    "fog/iot/energy": "Energy"
}

client_influx = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
write_api = client_influx.write_api(write_options=SYNCHRONOUS)

message_count = defaultdict(int)
start_time = defaultdict(lambda: time.time())

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        for topic in TOPIC_MEASUREMENT_PAIRS:
            client.subscribe(topic)

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    measurement = TOPIC_MEASUREMENT_PAIRS.get(msg.topic, "Unknown")
    current_time = time.time()

    sent_time = data.get('timestamp', current_time)
    latency_ms = (current_time - sent_time) * 1000

    point = Point(measurement).field("latency_ms", latency_ms)
    message_count[measurement] += 1

    if current_time - start_time[measurement] >= 60:
        throughput = message_count[measurement]
        point.field("throughput_msgs_per_min", throughput)
        message_count[measurement] = 0
        start_time[measurement] = current_time
    else:
        point.field("throughput_msgs_per_min", message_count[measurement])

    for k, v in data.items():
        if k != "timestamp":
            if isinstance(v, (int, float)):
                point.field(k, v)
            else:
                point.tag(k, str(v))

    write_api.write(INFLUXDB_BUCKET, INFLUXDB_ORG, point)

    sensor = data.get('sensor', 'N/A')
    kondisi = data.get('kondisi', 'N/A')
    power_usage = data.get('power_usage', 'N/A')

    print(f"""
📌 Data disimpan ke InfluxDB ({measurement}):
    ✅ Latency: {latency_ms:.2f} ms
    ✅ Throughput (pesan per menit): {message_count[measurement]}
    ✅ Power Usage: {power_usage} watt
    ✅ Sensor: {sensor}
    ✅ Kondisi: {kondisi}
""")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER, MQTT_PORT)
client.loop_forever()
