# ESP32-P4 Pin Connections for Doki OS

## Quick Reference Table

| Pin Type | Display 0 | Display 1 | Display 2 | Shared? | Location | Notes |
|----------|-----------|-----------|-----------|---------|----------|-------|
| VCC      | 3.3V/5V   | 3.3V/5V   | 3.3V/5V   | Yes     | Power    | 
| GND      | GND       | GND       | GND       | Yes     | Any GND  | 
| MOSI     | GPIO7     | GPIO7     | GPIO7     | **Yes** | Left 
| SCLK     | GPIO8     | GPIO8     | GPIO8     | **Yes** | Right 
| CS       | GPIO33    | GPIO32    | GPIO24    | No      | Mixed    | 
| DC       | GPIO21    | GPIO22    | GPIO25    | No      | Mixed    | 
| RST      | GPIO20    | GPIO23    | GPIO26    | No      | Left     | 

---

## Hardware Config Defines

```cpp
// SPI Pins (shared by all displays)
// WORKING SOLUTION: GPIO7/8 provides clean serial output (I2C pins repurposed for SPI)
#define SPI_MOSI_PIN                    7   // SDA (I2C) repurposed for SPI
#define SPI_SCLK_PIN                    8   // SCL (I2C) repurposed for SPI

// Display 0 Pins
#define DISPLAY_0_CS_PIN                33
#define DISPLAY_0_DC_PIN                21
#define DISPLAY_0_RST_PIN               20

// Display 1 Pins
#define DISPLAY_1_CS_PIN                32
#define DISPLAY_1_DC_PIN                22
#define DISPLAY_1_RST_PIN               23

// Display 2 Pins
#define DISPLAY_2_CS_PIN                24
#define DISPLAY_2_DC_PIN                25
#define DISPLAY_2_RST_PIN               26
```

---

## Physical Wiring for Display 0

```
ST7789 Display 0          ESP32-P4
─────────────────         ─────────────
VCC        ────────────►  3.3V (or 5V)
GND        ────────────►  GND
CS         ────────────►  GPIO33
DC (RS)    ────────────►  GPIO21
RST        ────────────►  GPIO20
SDA (MOSI) ────────────►  GPIO7 (I2C SDA) ✓
SCL (SCLK) ────────────►  GPIO8 (I2C SCL) ✓
BLK (LED)  ────────────►  3.3V (always on)
```

---

## Important Notes

### Pin Selection - Final Working Solution
- **GPIO7** and **GPIO8** are the WORKING pins (I2C SDA/SCL repurposed for SPI)
- ✓ **Clean Serial Output**: No gibberish during SPI transfers
- ✓ **Display Working**: Full functionality confirmed
- ✓ **Application Boots**: No boot failures

### Testing History
Pins tested and results:
- **GPIO37/38** (UART TX/RX): ⚠️ Worked but caused serial gibberish
- **GPIO45/46**: ❌ Boot failure (application wouldn't start)
- **GPIO47/48**: ❌ Boot failure (application wouldn't start)
- **GPIO53/54**: ❌ Boot failure (application wouldn't start)
- **GPIO7/8** (I2C): ✓ **WORKING SOLUTION** - Clean serial + working display

### Power
- Check if your ST7789 is 3.3V or 5V compatible
- Use adequate power supply: **5V 2A** minimum for 3 displays
- Each display draws ~50-100mA

### Wiring Tips
1. Keep wires **short** (<10cm for 40MHz)
2. **Twist** MOSI and SCLK together
3. Add **0.1µF capacitor** near each VCC pin
4. Ensure **common ground** for all displays

---

## SPI Speed Settings

| Speed   | Status      | Use Case                    |
|---------|-------------|-----------------------------|
| 40 MHz  | Safe        | Initial testing, stable     |
| 60 MHz  | Recommended | 1.5x faster, good for 30FPS |
| 80 MHz  | Maximum     | Very short wires only       |

---

## Test Program Checklist

- [ ] Connect Display 0 only first
- [ ] Verify power and GND connections
- [ ] Check all 7 connections (VCC, GND, CS, DC, RST, MOSI, SCLK)
- [ ] Run test program
- [ ] Look for display output
- [ ] Check serial monitor for errors

---

## Troubleshooting

**No output:** Check VCC, GND, and all pin connections
**Garbled display:** Reduce SPI frequency to 20MHz
**Intermittent:** Check power supply current capacity

---

## Solution Notes

### USB-JTAG Console Configuration
The ESP32-P4 uses USB-JTAG for console output (not traditional UART). Ensure your `sdkconfig` has:
```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ESP_CONSOLE_USB_CDC_SUPPORT_ETS_PRINTF=y
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
```

This routes `printf()` and `ESP_LOGI()` output through the USB-CDC interface for clean serial monitoring.

### Why GPIO7/8 Works
- I2C pins (SDA/SCL) can be repurposed for SPI on ESP32-P4
- No conflict with UART or other critical peripherals
- Provides stable SPI communication at 20MHz
- Allows clean serial debugging via USB-JTAG

---

Last Updated: 2025-01-29
Board: ESP32-P4-Function-EV-Board
Display: ST7789 SPI (240x320)
Status: ✓ Working with GPIO7/8 - Clean serial output confirmed