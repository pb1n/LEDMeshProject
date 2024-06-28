# LUMNET - Device Code (ARM Wireless Mesh LEDs Project)
### Third-year Group Project
This project deploys a mesh network (PainlessMesh) to connect nodes of Addressable LEDs to display animations across them in sync. A separate repository can be found [here](https://github.com/Shakofalltrades/ARM_LED_APP) for the server and website used to control the nodes.

#### Contributors (ESP32 Code):
- Janvid Wu
- Robin Masih

## Set Up
To set up the border-router and mesh node(s), at least **three** ESPs are required. **Two** for the border-router, and **one** per LED node. Currently, each node uses a 16 by 16 matrix of **WS2812B Addressable LEDs** (in serpentine configuration), although the grid size can be altered on the [LEDNode.ino](LEDNode/LEDNode.ino) by changing the following directives: `GRID_HEIGHT` and `GRID_WIDTH`. An **SHT41** sensor is also required for the sensor mode operation.
#### Optional - LEDs for the Border-router
To add status indication LEDs to the Border-router, 5 LEDs are required. Although optional, this is highly recommended.

### Arduino IDE
This project utilises Arduino code. To flash it, the following libraries are required:
- [ArduinoJson by Benoît Blanchon](https://github.com/bblanchon/ArduinoJson)
- [PainlessMesh by BlackEdder](https://gitlab.com/painlessMesh/painlessMesh)
- [Adafruit SHT4x by Adafruit](https://github.com/adafruit/Adafruit_SHT4x)
- [NeoPixelBus by Makuna](https://github.com/Makuna/NeoPixelBus)
- [LodePNG by Lode Vandevenne](https://github.com/lvandeve/lodepng?tab=Zlib-1-ov-file#readme)

### Border-router
You may wonder why two ESP32s are "*required*" for the border-router. This is done as each ESP32 only has one RF path. At any given time, only PainlessMesh or Wi-Fi can utilise the RF path. Using one ESP32 to do both tasks is possible, although this will degrade the quality of transmission (through latency) and affects the operation of the mesh network. As such, the border-router is divided into two halves, the **Gateway**, responsible for connecting to the wider internet, and the **Access Point**, responsible for connecting to the mesh network.

#### Wiring the UART Connection Between the ESP32s
To connect the two ESP32s, a UART connection is utilised. The pins must be wired as follows:
| Gateway (PIN) | Access Point (PIN) |
|---|---|
| 18 | 11 |
| 19 | 10 |
| 5V | 5V |
| GND | GND |

#### Optional - Wiring the LEDs for the Border-router
| Use | Gateway (PIN) | Access Point (PIN) |
| -- |---|---|
| Rx | 4 | - |
| Tx | 5 | - |
| Wi-Fi Status| 3 | - |
| WebSocket Status | 2 | - |
| Mesh Network | - | 5 |

The Rx and Tx status lights blink when data is received and transmitted, respectively. The Wi-Fi indicator will flash until a network is joined successfully, remaining illuminated. The WebSocket status light will illuminate when the connection is established. The mesh network indicator will illuminate upon successful connection/network establishment.

#### Flashing the ESP32s
Once wired, the set up is mostly complete. The only outstanding task before flashing is to modify the network addresses and local network credentials in [Gateway.ino](BorderRouter/Gateway/Gateway.ino).

Now the ESP32s can be flashed. Ensure that the correct code (Gateway or Access Point) is flashed to corresponding ESP32.

#### Debugging Mode
A directive, aptly named  `DEBUGGING_MODE`, will skip the Wi-Fi and WebSocket set up. The serial monitor can then be used to send commands to the nodes.

### Node Devices
#### Wiring the LEDs and Sensors
The table below indicates how the LEDs should be wired to the ESP32.
| ESP32 | LED Matrix |
|---|---|
| 5V | 5V |
| 2 | DIN |
| GND | GND |

The table below indicates how the SHT41 sensor should be wired to the ESP32.
| ESP32 | LED Matrix |
|---|---|
| 5V | 5V |
| 2 | DIN |
| GND | GND |

#### Flashing the File System
Before flashing the code to the ESP, the file system needs to be flashed. The file system used is LittleFS. The upload tool (for Arduino IDE 2.2.1 or higher) can be found [here](https://github.com/earlephilhower/arduino-littlefs-upload).

#### Flashing the ESP32s
The code for the node device does not need modification (unless the Access Point's mesh network credentials were changed). It is ready to be flashed.
