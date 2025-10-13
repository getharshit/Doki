/**
 * ST7789 Display Driver for ESP32-S3 with LVGL
 * Handles SPI communication and display initialization
 */

#ifndef ST7789_DRIVER_H
#define ST7789_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>

// Pin definitions for Waveshare ESP32-S3
#define TFT_MOSI 37
#define TFT_SCLK 36
#define TFT_CS   33
#define TFT_DC   15
#define TFT_RST  16

// Display dimensions
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ST7789 Commands
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_NORON   0x13
#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

// SPI settings
#define SPI_FREQUENCY 40000000  // 40MHz

class ST7789_Driver {
private:
    SPIClass* spi;
    
    // Helper functions for SPI communication
    inline void writeCommand(uint8_t cmd) {
        digitalWrite(TFT_DC, LOW);   // Command mode
        digitalWrite(TFT_CS, LOW);   // Select display
        spi->transfer(cmd);
        digitalWrite(TFT_CS, HIGH);  // Deselect
    }
    
    inline void writeData(uint8_t data) {
        digitalWrite(TFT_DC, HIGH);  // Data mode
        digitalWrite(TFT_CS, LOW);   // Select display
        spi->transfer(data);
        digitalWrite(TFT_CS, HIGH);  // Deselect
    }
    
    inline void writeData16(uint16_t data) {
        digitalWrite(TFT_DC, HIGH);  // Data mode
        digitalWrite(TFT_CS, LOW);   // Select display
        spi->transfer16(data);
        digitalWrite(TFT_CS, HIGH);  // Deselect
    }

public:
    ST7789_Driver() {
        spi = &SPI;
    }
    
    void init() {
        // Initialize pins
        pinMode(TFT_CS, OUTPUT);
        pinMode(TFT_DC, OUTPUT);
        pinMode(TFT_RST, OUTPUT);
        
        digitalWrite(TFT_CS, HIGH);
        digitalWrite(TFT_DC, HIGH);
        
        // Initialize SPI
        spi->begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
        spi->setFrequency(SPI_FREQUENCY);
        spi->setDataMode(SPI_MODE0);
        spi->setBitOrder(MSBFIRST);
        
        // Hardware reset
        digitalWrite(TFT_RST, HIGH);
        delay(10);
        digitalWrite(TFT_RST, LOW);
        delay(20);
        digitalWrite(TFT_RST, HIGH);
        delay(150);
        
        // Software reset
        writeCommand(ST7789_SWRESET);
        delay(150);
        
        // Sleep out
        writeCommand(ST7789_SLPOUT);
        delay(120);
        
        // Color mode: 16-bit color (RGB565)
        writeCommand(ST7789_COLMOD);
        writeData(0x55);  // 16-bit/pixel
        delay(10);
        
        // Memory data access control (rotation)
        writeCommand(ST7789_MADCTL);
        writeData(0x00);  // Portrait mode, RGB order
        
        // Normal display on
        writeCommand(ST7789_NORON);
        delay(10);
        
        // Display on
        writeCommand(ST7789_DISPON);
        delay(100);
        
        Serial.println("ST7789 display initialized!");
    }
    
    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
        // Column address set
        writeCommand(ST7789_CASET);
        writeData16(x0);
        writeData16(x1);
        
        // Row address set
        writeCommand(ST7789_RASET);
        writeData16(y0);
        writeData16(y1);
        
        // Write to RAM
        writeCommand(ST7789_RAMWR);
    }
    
    void pushColors(uint16_t* colors, uint32_t len) {
        digitalWrite(TFT_DC, HIGH);  // Data mode
        digitalWrite(TFT_CS, LOW);   // Select display
        
        // Transfer pixel data
        for (uint32_t i = 0; i < len; i++) {
            spi->transfer16(colors[i]);
        }
        
        digitalWrite(TFT_CS, HIGH);  // Deselect
    }
    
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
        if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT)) return;
        if ((x + w - 1) >= TFT_WIDTH) w = TFT_WIDTH - x;
        if ((y + h - 1) >= TFT_HEIGHT) h = TFT_HEIGHT - y;
        
        setAddrWindow(x, y, x + w - 1, y + h - 1);
        
        digitalWrite(TFT_DC, HIGH);  // Data mode
        digitalWrite(TFT_CS, LOW);   // Select display
        
        uint32_t num = (uint32_t)w * h;
        for (uint32_t i = 0; i < num; i++) {
            spi->transfer16(color);
        }
        
        digitalWrite(TFT_CS, HIGH);  // Deselect
    }
    
    void fillScreen(uint16_t color) {
        fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
    }
};

// Global driver instance
extern ST7789_Driver display_driver;

// LVGL flush callback
void lvgl_flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);

#endif // ST7789_DRIVER_H