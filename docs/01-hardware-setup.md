# Hardware Setup

## Required Components

### 1. ESP32-S3 Development Board
- **Recommended**: Waveshare ESP32-S3R2
- **Specifications**:
  - Dual-core Xtensa LX7 @ 240MHz
  - 512KB SRAM
  - 2MB PSRAM (QSPI mode)
  - 16MB Flash
  - Built-in WiFi (2.4GHz) and Bluetooth 5.0
  - USB-C for programming and power

### 2. ST7789 TFT Displays (x2)
- **Resolution**: 240x320 pixels
- **Interface**: SPI
- **Color Depth**: RGB565 (16-bit color)
- **Controller**: ST7789V
- **Backlight**: Built-in LED backlight

### 3. Additional Components
- USB-C cable for programming and power
- Breadboard or custom PCB for connections
- Jumper wires (Male-to-Female recommended)
- 5V/2A power supply (if not using USB power)

## Pin Configuration

### Shared SPI Bus

The displays share the SPI MOSI and SCLK pins for efficient use of GPIO pins.

| Signal | ESP32-S3 Pin | Description |
|--------|--------------|-------------|
| MOSI   | GPIO 37      | Master Out Slave In (data) |
| SCLK   | GPIO 36      | Serial Clock |

### Display 0 (Primary)

| Signal | ESP32-S3 Pin | ST7789 Pin | Description |
|--------|--------------|------------|-------------|
| CS     | GPIO 33      | CS         | Chip Select |
| DC     | GPIO 15      | DC/RS      | Data/Command |
| RST    | GPIO 16      | RST        | Reset |
| BL     | 3.3V         | BL         | Backlight (always on) |
| VCC    | 3.3V         | VCC        | Power |
| GND    | GND          | GND        | Ground |

### Display 1 (Secondary)

| Signal | ESP32-S3 Pin | ST7789 Pin | Description |
|--------|--------------|------------|-------------|
| CS     | GPIO 34      | CS         | Chip Select |
| DC     | GPIO 17      | DC/RS      | Data/Command |
| RST    | GPIO 18      | RST        | Reset |
| BL     | 3.3V         | BL         | Backlight (always on) |
| VCC    | 3.3V         | VCC        | Power |
| GND    | GND          | GND        | Ground |

## Wiring Diagram

```
ESP32-S3                    Display 0              Display 1
┌─────────┐                ┌─────────┐            ┌─────────┐
│         │                │         │            │         │
│ GPIO 37 ├────────┬───────┤ MOSI    │            │ MOSI    │
│ GPIO 36 ├────────┼───┬───┤ SCLK    │            │ SCLK    │
│         │        │   │   │         │            │         │
│ GPIO 33 ├────────┼───┼───┤ CS      │            │         │
│ GPIO 15 ├────────┼───┼───┤ DC      │            │         │
│ GPIO 16 ├────────┼───┼───┤ RST     │            │         │
│         │        │   │   │         │            │         │
│ GPIO 34 ├────────┼───┼───┼─────────┼────────────┤ CS      │
│ GPIO 17 ├────────┼───┼───┼─────────┼────────────┤ DC      │
│ GPIO 18 ├────────┼───┼───┼─────────┼────────────┤ RST     │
│         │        │   │   │         │            │         │
│ 3.3V    ├────────┼───┼───┤ VCC     │            │ VCC     │
│ 3.3V    ├────────┼───┼───┤ BL      │            │ BL      │
│ GND     ├────────┼───┼───┤ GND     │            │ GND     │
│         │        │   │   │         │            │         │
└─────────┘        │   └───┴─────────┴────────────┴─────────┘
                   │       (Shared SCLK)
                   └───────────────────────────────┘
                          (Shared MOSI)
```

## Assembly Steps

### Step 1: Prepare the ESP32-S3 Board
1. Ensure the board is unplugged from power
2. Inspect for any physical damage
3. Verify all GPIO pins are accessible

### Step 2: Connect Shared SPI Signals
1. Connect GPIO 37 (MOSI) to both displays' MOSI pins
2. Connect GPIO 36 (SCLK) to both displays' SCLK pins
3. These lines are shared between both displays

### Step 3: Connect Display 0
1. CS: GPIO 33 → Display 0 CS
2. DC: GPIO 15 → Display 0 DC
3. RST: GPIO 16 → Display 0 RST
4. VCC: 3.3V → Display 0 VCC
5. GND: GND → Display 0 GND
6. BL: 3.3V → Display 0 BL (backlight)

### Step 4: Connect Display 1
1. CS: GPIO 34 → Display 1 CS
2. DC: GPIO 17 → Display 1 DC
3. RST: GPIO 18 → Display 1 RST
4. VCC: 3.3V → Display 1 VCC
5. GND: GND → Display 1 GND
6. BL: 3.3V → Display 1 BL (backlight)

### Step 5: Verify Connections
- Double-check all connections match the pinout table
- Ensure no short circuits between adjacent pins
- Verify power and ground connections

### Step 6: Connect USB Power
1. Connect USB-C cable to ESP32-S3
2. Connect to computer for programming
3. OR connect to 5V/2A power supply for standalone operation

## Power Considerations

### Power Budget

| Component | Current Draw | Notes |
|-----------|--------------|-------|
| ESP32-S3  | ~200mA       | Peak during WiFi transmission |
| Display 0 | ~100mA       | With backlight at full brightness |
| Display 1 | ~100mA       | With backlight at full brightness |
| **Total** | **~400mA**   | Typical operation |

### Power Sources

**Option 1: USB-C Power (Recommended for Development)**
- Connect USB-C cable to computer or 5V/2A USB adapter
- Sufficient for dual display operation
- Allows programming and serial monitoring

**Option 2: External 5V Supply**
- Use regulated 5V/2A power supply
- Connect to ESP32-S3 5V pin (or USB port)
- Required for standalone operation

**Important**: The ESP32-S3 has an onboard 3.3V regulator that powers the displays. Ensure the 5V input can provide at least 500mA.

## Physical Mounting

### Display Positioning
- Position displays side-by-side for dual-screen effect
- Maintain ~5mm gap between displays
- Use standoffs or 3D-printed brackets for stability

### Cable Management
- Keep SPI signal wires short (<20cm) for signal integrity
- Route power and ground separately from signal lines
- Use cable ties or clips to organize wiring

## Testing the Hardware

### Initial Power-On Test
1. Connect power (displays should light up with backlight)
2. No image is normal at this stage (firmware not loaded)
3. ESP32-S3 LED should blink during boot

### After Flashing Firmware
1. Both displays should initialize (brief white flash)
2. Display 0 should show Clock app
3. Display 1 should show Weather app
4. Check serial monitor for status messages

### Troubleshooting Hardware Issues

**Displays not lighting up:**
- Check power connections (VCC, GND, BL)
- Verify 3.3V is present at display VCC pins
- Test backlight by measuring voltage at BL pin

**Displays light up but show no content:**
- Verify SPI connections (MOSI, SCLK)
- Check CS, DC, RST pins are connected correctly
- Ensure pin numbers in code match physical wiring

**Only one display works:**
- Check individual CS, DC, RST pins for the non-working display
- Verify shared SPI lines are connected to both displays
- Swap displays to isolate hardware vs. software issue

**Garbled or incorrect colors:**
- Check data line connections (MOSI)
- Verify SCLK signal integrity
- Ensure firmware color format matches display (RGB565)

**Random freezes or reboots:**
- Insufficient power supply (upgrade to 2A adapter)
- Check for loose connections
- Verify all GND connections are solid

## Safety Considerations

- **ESD Protection**: Handle ESP32-S3 with care, avoid static discharge
- **Power Rating**: Do not exceed 3.3V on display VCC pins
- **Current Limiting**: Use appropriate power supply (2A minimum)
- **Heat Management**: Ensure adequate ventilation; ESP32-S3 may warm during WiFi operations

## Next Steps

After hardware assembly and testing:
1. Proceed to [Platform Configuration](./02-platform-configuration.md) for software setup
2. Flash the firmware using PlatformIO
3. Test basic functionality with default apps

## Schematic Reference

For PCB design or advanced integrations, refer to:
- ESP32-S3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- ST7789 controller datasheet: Available from display manufacturer

## Additional Notes

### SPI Bus Speed
- Configured for 40MHz (see `platformio.ini`)
- Can be reduced to 20MHz if signal integrity issues occur
- Higher speeds require shorter wires and better signal routing

### Future Expansion
- GPIO pins available for additional peripherals (buttons, sensors, etc.)
- I2C bus available for additional sensors
- Second SPI bus (VSPI) available if needed

---

[← Back to Main](./README.md) | [Next: Platform Configuration →](./02-platform-configuration.md)
