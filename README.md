# Bier.Cool-Firmware
This page is all about the famous firmware for the fridge sensor to keep track of the available beers

## Detect address Dallas ds18b20
~~Each sensor has his own address, it is possible to add more ds18b20's.
To find them the address upload the code from findAddressTempSensor.ino
Once uploaded connect to serial monitor and copy the addresses, like 0x28, 0xE2, 0x1B, 0x77, 0x91, 0x6, 0x2, 0xEE~~
The code has been adjusted so that it scans for available devices. When multiple sensors are attached all get read.
All the sensors will be used in the Json payload message.

## Bier.Cool firmware
The Wemos D1 mini is being used for this project. The temperature sensor is connect on D2(equals GPIO4) and the limit switch on
D3(equals GPIO0).

- Paste the address of the temperature sensor on row 24
- Enter the address of the MQTT server on row 29
- Enter your Wi-Fi network name and password on row 33, 34
- If you have multiple sensors adjust the MQTT payload message

### Todo
- [x] Read temperature and send to MQTT
- [x] Incorporate limit switch
- [x] Create methods for MQTT message payload
- [x] Create methods for dynamic sensors detection
- [ ] Make code pretty
