# LUMNET (ARM Wireless Mesh LEDs Project)
### Third-year Group Project
#### Contributors (ESP32 Code):
- Janvid Wu
- Robin Masih

## Set-up
To set up the border-router and mesh node(s), at least **three** ESPs are required. **Two** for the border-router, and **one** per LED node. Currently, each node uses a 16 by 16 matrix of **WS2812B Addressable LEDs**, although the grid size can be altered on the [NodeMesh.ino](NodeMesh/NodeMesh.ino) by changing the following directives: **GRID_HEIGHT** and **GRID_WIDTH**.

### Border-router
You may wonder why two ESP32s are "*required*" for the border-router. This is done as each ESP32 only has one RF path. At any given time, only PainlessMesh or Wi-Fi can utilise the RF path. Using one ESP32 to do both tasks is possible, although this will degrade the quality of transmission (through latency) and affects the operation of the mesh network. As such, the border-router is divided into two halves, the **Gateway**, responsible for connecting to the wider internet, and the **Access Point**, responsible for connecting to the mesh network.

#### Wiring the UART Connection Between the ESP32s
To connect the two ESP32s, a UART connection is utilised. The pins must be wired as follows:
| Gateway | Access Point |
|---|---|
| 18 | 11 |
| 19 | 10 |


