# Matter ESP32 Modbus Adapter
The goal of this project is to provide a simple working Modbus adapter that can expose devices over Matter. It is designed for the ESP32 MCU.

# SDM120M - Electrical Sensor

The first device supported by this project is the Eastron SDM120M Single Phase Energy Meter

The readings from this device will be exposed using as a Matter Electrical Sensor Device Type.

# Hardware - Seeed XIAO ESP32-C6

The GPIOs in code has been selected for the XIAO ESP32-C6. 

| GPIO | Connection |
|------|------------|
| 16 | MAX485 - DI   | 
| 17 | MAX485 - RO   |
| 21 | MAX485 - DE/RE|
| 22 | OLED - SDA |
| 23 | OLED - SCL |
| 2  | OLED - RESET |

Ensure there is a 120kΩ across PINS A&B of the MAX485.
