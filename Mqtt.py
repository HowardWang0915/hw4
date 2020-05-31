import matplotlib.pyplot as plt
import numpy as np
import paho.mqtt.client as paho
import time

t = np.arange(0, 20, 0.1) # time vector; create Fs samples between 0 and 10.0 sec.
x = np.arange(0, 1, 1 / 200) # signal vector; create Fs samples
y = np.arange(0, 1, 1 / 200) # signal vector; create Fs samples
z = np.arange(0, 1, 1 / 200) # signal vector; create Fs samples
log = np.arange(0, 1, 1 / 200) # log data; create Fs samples

mqttc = paho.Client()

# Settings for connection
host = "localhost"
topic= "Mbed"
port = 1883

# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    data = str(msg.payload)
    data = data.split()
    x[0] = 0
    for i in range(1, 200):
        x[i] = float(data[i])
    # y
    for i in range(0, 200):
        y[i] = float(data[i + 200])
    # z
    for i in range(0, 200):
        z[i] = float(data[i + 400])
    # log
    for i in range(0, 200):
        log[i] = float(data[i + 600])

# plot
    fig, ax = plt.subplots(2, 1)
    ax[0].plot(t, x, label = 'x')
    ax[0].plot(t, y, label = 'y')
    ax[0].plot(t, z, label = 'z')
    ax[0].set_ylabel('Acc Vector')
    ax[0].set_xlabel('Time')
    ax[0].legend()     # add legend
    ax[1].stem(t, log, use_line_collection = True)
    ax[1].set_ylabel('Tilt')
    ax[1].set_xlabel('Time')
    plt.show()

def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed OK")

def on_unsubscribe(mosq, obj, mid, granted_qos):
    print("Unsubscribed OK")
# Set callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_unsubscribe = on_unsubscribe

# Connect and subscribe
print("Connecting to " + host + "/" + topic)
mqttc.connect(host, port=1883, keepalive=60)
mqttc.subscribe(topic, 0)

# while(1):
#     mesg = "Hello, world!"
#     mqttc.publish(topic, mesg)
#     print(mesg)
#     time.sleep(1)
mqttc.loop_forever()
