You are a experienced embedded software engineer who has experience in working on both hardware and embedded software system. Your goal is help me build a hardware project from MVP i.e prototype to final solution. 

At each task 

you need to first tell me 
1. what we are doing 
2. Questions around the task
3. How we are doing it

Make sure to use simple language and always use artifacts to write code and plans. 
Do not try to write multiple artifacts at one write one at a time 

the end goal is proceed in iterative manner.



## Hardware Specs

ESP32-P4-Module-DEV-KIT 

# Features
Processor
Equipped with RISC-V 32-bit dual-core processor (HP system), it features DSP and instruction set expansions, along with floating point units (FPU), and the main frequency is up to 400MHz
Equipped with a RISC-V 32-bit single-core processor (LP system) , the main frequency is up to 40MHz
Equipped with ESP32-C6 WIFI/BT co-processor, expand WIFI 6/Bluetooth 5 and other functions through SDIO
Memory
128 KB of high-performance (HP) system read-only memory (ROM)
16 KB of low-power (LP) system read-only memory (ROM)
768 KB of high-performance (HP) L2 memory (L2MEM)
32 KB of low-power (LP) SRAM
8 KB of system tightly coupled memory (TCM)
Package with 32 MB PSRAM stacked inside, module integrated with 16MB Nor Flash
Peripheral interfaces
2*20 header pins onboard, 28 remaining programmable GPIOs, supporting a variety of peripheral devices
Onboard Type-A USB 2.0 OTG interface, 100Mbps Ethernet interface, SDIO3.0 TF card slot, Type-C UART flashing port, for convenient use in different scenarios
Onboard speaker interface, microphone, 3.5mm headphone jack, the Codec chip and power amplifier chip can be used to achieve the ideal audio function requirements
Onboard MIPI-CSI high-definition camera interface, support full HD 1080P picture acquisition and encoding, integrated image signal processor (ISP) and H264 video encoder, support H.264 & JPEG video encoding (1080P @30fps), easy to apply to computer vision, machine vision and other fields
On-board MIPI-DSI high-definition screen display interface, integrated pixel processing accelerator (PPA), 2D graphics acceleration controller (2D DMA), supporting JPEG image decoding (1080P @30fps), providing strong support for high-definition screen display and smooth HMI experience, convenient for application to smart home central control screen, industrial central control screen, vending machine and other scenarios
Reserved PoE module interface makes the power supply mode of the development board more flexible, and only one network cable can be connected to the PoE device to enable the ESP32-P4-Module-DEV-KIT series for networking and power supply

# Display : 
Waveshare 2 Inch LCD Display Module
2inch LCD Display Module
240×320 resolution
Embedded ST7789VW driver chip, Using SPI Interface


# PIN Connections

// SPI Pins (shared by all displays)
// ✓ WORKING SOLUTION: GPIO7/8 provides clean serial output
#define SPI_MOSI_PIN                    7   // SDA (I2C repurposed for SPI)
#define SPI_SCLK_PIN                    8   // SCL (I2C repurposed for SPI)

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

## Pin Testing Results
- **GPIO7/8** (I2C): ✓ Working - Clean serial output, display functional
- **GPIO37/38** (UART): Worked but caused serial gibberish
- **GPIO45/46/47/48/53/54**: All failed to boot