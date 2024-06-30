import paho.mqtt.client as mqtt

# MQTT broker callback functions
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    client.subscribe("test/topic")

def on_message(client, userdata, msg):
    print(f"Received message: {msg.payload.decode()} on topic {msg.topic}")

# Create an MQTT client instance with version 5.0
client = mqtt.Client(protocol=mqtt.MQTTv5)

# Attach the callback functions
client.on_connect = on_connect
client.on_message = on_message

# Connect to the broker using the public IP address
broker_address = "46.2.23.249"  # Replace with your public IP address
broker_port = 1883

client.connect(broker_address, broker_port, 60)

# Start the MQTT client loop
client.loop_forever()
