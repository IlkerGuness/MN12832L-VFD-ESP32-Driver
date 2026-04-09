# MN12832L-VFD-ESP32-Driver

Just a quick project to bring this old Japanese VFD back to life using an ESP32. 

### ⚡ What's inside?
- **Basic Driver:** Solving the brightness issues with Dual-Grid logic.
- **Cool Animations:** A Cyberpunk-style Matrix rain and a "Zogolder" logo reveal.
- **Non-blocking code:** So the screen won't burn out while waiting for delays.

## 🛠 Hardware & Pinout

This is how you should wire up the MN12832L to your ESP32. Double-check your connections before powering up the HV (High Voltage) line!
**Also, don't forget to share the grounds!**

| VFD Pin Label | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **SIN** | 23 | SPI MOSI |
| **CLK** | 18 | SPI SCK |
| **LAT** | 22 | Data Latch |
| **BLK** | 21 | PWM |
| **EF** | 5V+ | Enable Filament |
| **HV** | 5V+ | High Voltage Enable |

*Note: Don't forget to connect your VCC (Logic 5V), GND, and the specific Filament/HV power supplies required by the VFD hardware.*

*Note2: If you have exterel 5V, use it because sometimes ESP32 5V+ line can not be enough.*

## 📺 See it in Action
Check out the "Zogolder" animation and the Matrix rain effect here:

https://github.com/user-attachments/assets/0654cc59-3293-4e5f-9eab-047a94d7fbef

*Note: I’m not a coding pro, I built this with the help of Gemini. Documentation is thin because I'm short on time, but the code is commented!*
