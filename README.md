# esp-now-gw-mqtt

This is platform.io project for esp8266 device.

## Background

This is one part of home automation project which can control relay(s) over
* esp-now message from paired device (remote control)
* esp-now message from mqtt<->esp-now bridge

## Included components
* __esp-now-gw-mqtt__ - mqtt<->esp-now bridge - wifi and mqtt part (this repo)
* esp-now-gw-transmitter - mqtt<->esp-now bridge - esp-now part
* esp-now-relay - controls relay to open gate or do another action
* esp-now-relay-remote - battery powered ESP32-C6 device to control relay e.g. from car

## Flashing / Uploading

Two environments are defined in `platformio.ini`:
* `upesy_wroom` — USB serial (default, port `COM6`)
* `ota` — Over-The-Air (device IP `192.168.40.50`)

### Firmware

```bash
# Serial
pio run -t upload -e upesy_wroom

# OTA
pio run -t upload -e ota
```

### Filesystem (LittleFS — web UI files in `data/`)

Upload the filesystem separately whenever you change files in the `data/` directory.
The `upload` target only flashes the firmware and does **not** touch the filesystem partition.

```bash
# Serial
pio run -t uploadfs -e upesy_wroom

# OTA
pio run -t uploadfs -e ota
```

---

## Example mqtt messages

Following is shell script with typical messages used for testing these components

```bash
# mac address is entered without colons
GW_MAC=102030405060

# mqtt host
HOST=192.168.1.1

# pairing magic - constant secret used for relay module pairing 
PAIR_MAGIC=12345

# send relay push (press and release for 500ms)
mosquitto_pub -h $HOST -t "/esp-now/gw/send" -m "{\"to\":\"${GW_MAC}\",\"message\":{\"channel\":1, \"push\":500}}"

# swicth relay module to OTA mode for 30s
mosquitto_pub -h $HOST -t "/esp-now/gw/send" -m "{\"to\":\"${GW_MAC}\",\"message\":{\"otaMs\":30000}}"

# swicth relay module to pairing on relay 1 for 30s
mosquitto_pub -h $HOST -t "/esp-now/gw/send" -m "{\"to\":\"${GW_MAC}\",\"message\":{\"pairMs\":30000, \"channel\":1}}"

# ask relay module to send list of paired clients
mosquitto_pub -h $HOST -t "/esp-now/gw/send" -m "{\"to\":\"${GW_MAC}\",\"message\":{\"pairMagic\":$PAIR_MAGIC, \"action\": \"list\"}}"

# delete relay module pairing at index 0
mosquitto_pub -h $HOST -t "/esp-now/gw/send" -m "{\"to\":\"${GW_MAC}\",\"message\":{\"pairMagic\":$PAIR_MAGIC, \"action\": \"unpair\", \"index\": 0}}"

# delete all relay module pairings 
mosquitto_pub -h $HOST -t "/esp-now/gw/send" -m "{\"to\":\"${GW_MAC}\",\"message\":{\"pairMagic\":$PAIR_MAGIC, \"action\": \"unpair\"}}"
```

