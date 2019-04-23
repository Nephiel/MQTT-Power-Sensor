# MQTT Mains Power Sensor
The sensor uses a Wemos D1 Mini and a Non-invasive Split Core Current Transformer type 100A SCT-013-000, available on eBay.
The sensor samples the current value every 5 seconds and publishes it to a topic via an MQTT broker.
A watchdog is also provided that kicks in after a few minutes if it cannot publish MQTT messages successfully.
If the connection fails, it attempts to reconnect to the WiFi and/or the MQTT Broker as needed.

Original Gerber files for the PCB layout and the source files for Kicad are included, although there may be issues with the custom libraries used.
