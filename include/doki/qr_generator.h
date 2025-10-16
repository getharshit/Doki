/**
 * @file qr_generator.h
 * @brief QR Code Generator for Doki OS - Visual setup assistance
 *
 * Generates and displays QR codes on LVGL displays to help users
 * quickly connect to the setup portal. QR codes can contain the
 * WiFi SSID, password, and setup URL.
 *
 * Features:
 * - Generate QR codes for WiFi and URLs
 * - Display on LVGL screens
 * - Configurable size and position
 * - WiFi QR code format (WIFI:S:ssid;T:WPA;P:password;;)
 *
 * Example:
 *   QRGenerator::displaySetupQR(screen, "DokiOS-Setup", "doki1234");
 */

#ifndef DOKI_QR_GENERATOR_H
#define DOKI_QR_GENERATOR_H

#include <Arduino.h>
#include <lvgl.h>
#include <qrcode.h>

namespace Doki {

/**
 * @brief QR Code Generator - Create and display QR codes
 *
 * Static class for QR code generation and display
 */
class QRGenerator {
public:
    /**
     * @brief Display setup QR code on screen
     *
     * @param screen LVGL screen object
     * @param ssid WiFi AP SSID
     * @param password WiFi AP password
     * @param url Setup URL (optional, auto-generated if empty)
     * @return LVGL canvas object containing QR code
     *
     * Creates a QR code that encodes WiFi connection information.
     * Scanning with a phone will automatically connect to the AP.
     *
     * Example:
     *   lv_obj_t* qr = QRGenerator::displaySetupQR(
     *       lv_scr_act(),
     *       "DokiOS-Setup",
     *       "doki1234"
     *   );
     */
    static lv_obj_t* displaySetupQR(lv_obj_t* screen,
                                      const String& ssid,
                                      const String& password,
                                      const String& url = "");

    /**
     * @brief Display URL QR code on screen
     *
     * @param screen LVGL screen object
     * @param url URL to encode
     * @param x X position (default: center)
     * @param y Y position (default: center)
     * @param scale Scale factor (default: 3)
     * @return LVGL canvas object containing QR code
     *
     * Creates a QR code for a URL. Scanning opens the URL in browser.
     *
     * Example:
     *   QRGenerator::displayURLQR(
     *       lv_scr_act(),
     *       "http://192.168.4.1/setup"
     *   );
     */
    static lv_obj_t* displayURLQR(lv_obj_t* screen,
                                    const String& url,
                                    int16_t x = -1,  // -1 means center
                                    int16_t y = -1,  // -1 means center
                                    uint8_t scale = 3);

    /**
     * @brief Display WiFi QR code on screen
     *
     * @param screen LVGL screen object
     * @param ssid WiFi network name
     * @param password WiFi password
     * @param security Security type (default: "WPA")
     * @param x X position
     * @param y Y position
     * @param scale Scale factor
     * @return LVGL canvas object containing QR code
     *
     * Creates a QR code using WiFi format. Compatible with most
     * Android and iOS devices for automatic WiFi connection.
     *
     * Format: WIFI:S:ssid;T:WPA;P:password;;
     */
    static lv_obj_t* displayWiFiQR(lv_obj_t* screen,
                                     const String& ssid,
                                     const String& password,
                                     const String& security = "WPA",
                                     int16_t x = -1,  // -1 means center
                                     int16_t y = -1,  // -1 means center
                                     uint8_t scale = 3);

    /**
     * @brief Generate QR code data
     *
     * @param data String to encode
     * @param qrcode Output: QRCode object
     * @param qrcodeData Output: QR code byte array
     * @return true if generation succeeded
     *
     * Low-level QR code generation. Use display functions for
     * easy integration with LVGL.
     */
    static bool generateQRCode(const String& data,
                                QRCode& qrcode,
                                uint8_t* qrcodeData);

    /**
     * @brief Create setup instruction screen
     *
     * @param screen LVGL screen object
     * @param ssid WiFi AP SSID
     * @param password WiFi AP password
     * @param url Setup URL
     *
     * Creates a complete setup screen with QR code, instructions,
     * and styling. Shows both QR code and text instructions.
     *
     * Example:
     *   QRGenerator::createSetupScreen(
     *       lv_scr_act(),
     *       "DokiOS-Setup",
     *       "doki1234",
     *       "http://192.168.4.1/setup"
     *   );
     */
    static void createSetupScreen(lv_obj_t* screen,
                                    const String& ssid,
                                    const String& password,
                                    const String& url);

private:
    // Default QR code version (size)
    static const uint8_t QR_VERSION = 4;  // 33x33 modules

    // Helper: Draw QR code on LVGL canvas
    static lv_obj_t* drawQROnCanvas(lv_obj_t* screen,
                                     QRCode& qrcode,
                                     int16_t x,
                                     int16_t y,
                                     uint8_t scale);

    // Helper: Generate WiFi QR string
    static String generateWiFiQRString(const String& ssid,
                                        const String& password,
                                        const String& security);
};

} // namespace Doki

#endif // DOKI_QR_GENERATOR_H
