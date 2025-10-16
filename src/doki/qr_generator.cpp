/**
 * @file qr_generator.cpp
 * @brief Implementation of QRGenerator
 */

#include "doki/qr_generator.h"

namespace Doki {

// ========================================
// Public Methods
// ========================================

lv_obj_t* QRGenerator::displaySetupQR(lv_obj_t* screen,
                                        const String& ssid,
                                        const String& password,
                                        const String& url) {
    Serial.println("[QRGenerator] Creating setup QR code...");

    // Use WiFi QR format for easy connection
    return displayWiFiQR(screen, ssid, password, "WPA", -1, -1, 3);
}

lv_obj_t* QRGenerator::displayURLQR(lv_obj_t* screen,
                                     const String& url,
                                     int16_t x,
                                     int16_t y,
                                     uint8_t scale) {
    Serial.printf("[QRGenerator] Creating URL QR code: %s\n", url.c_str());

    // Generate QR code
    uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];
    QRCode qrcode;

    if (!generateQRCode(url, qrcode, qrcodeData)) {
        Serial.println("[QRGenerator] ✗ Failed to generate QR code");
        return nullptr;
    }

    // Draw on canvas
    lv_obj_t* canvas = drawQROnCanvas(screen, qrcode, x, y, scale);

    Serial.println("[QRGenerator] ✓ URL QR code created");
    return canvas;
}

lv_obj_t* QRGenerator::displayWiFiQR(lv_obj_t* screen,
                                       const String& ssid,
                                       const String& password,
                                       const String& security,
                                       int16_t x,
                                       int16_t y,
                                       uint8_t scale) {
    Serial.printf("[QRGenerator] Creating WiFi QR code: SSID='%s'\n", ssid.c_str());

    // Generate WiFi QR string
    String wifiQRString = generateWiFiQRString(ssid, password, security);

    // Generate QR code
    uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];
    QRCode qrcode;

    if (!generateQRCode(wifiQRString, qrcode, qrcodeData)) {
        Serial.println("[QRGenerator] ✗ Failed to generate QR code");
        return nullptr;
    }

    // Draw on canvas
    lv_obj_t* canvas = drawQROnCanvas(screen, qrcode, x, y, scale);

    Serial.println("[QRGenerator] ✓ WiFi QR code created");
    return canvas;
}

bool QRGenerator::generateQRCode(const String& data,
                                  QRCode& qrcode,
                                  uint8_t* qrcodeData) {
    // Initialize QR code
    int status = qrcode_initText(&qrcode, qrcodeData, QR_VERSION, ECC_LOW, data.c_str());

    if (status != 0) {
        Serial.printf("[QRGenerator] Error: QR code init failed (status=%d)\n", status);
        return false;
    }

    return true;
}

void QRGenerator::createSetupScreen(lv_obj_t* screen,
                                     const String& ssid,
                                     const String& password,
                                     const String& url) {
    Serial.println("[QRGenerator] Creating complete setup screen...");

    // Clear screen
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    // Title
    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "Setup Required");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x667eea), 0);

    // QR Code (WiFi connection)
    lv_obj_t* qr = displayWiFiQR(screen, ssid, password, "WPA", -1, 50, 2);
    if (qr) {
        lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 40);
    }

    // Instructions
    lv_obj_t* inst1 = lv_label_create(screen);
    lv_label_set_text(inst1, "1. Scan QR code to connect");
    lv_obj_align(inst1, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_font(inst1, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(inst1, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t* inst2 = lv_label_create(screen);
    lv_label_set_text(inst2, "2. Configure WiFi");
    lv_obj_align(inst2, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_text_font(inst2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(inst2, lv_color_hex(0xFFFFFF), 0);

    // WiFi info
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "SSID: %s", ssid.c_str());
    lv_obj_t* ssidLabel = lv_label_create(screen);
    lv_label_set_text(ssidLabel, infoText);
    lv_obj_align(ssidLabel, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_text_font(ssidLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(ssidLabel, lv_color_hex(0x888888), 0);

    snprintf(infoText, sizeof(infoText), "Password: %s", password.c_str());
    lv_obj_t* passLabel = lv_label_create(screen);
    lv_label_set_text(passLabel, infoText);
    lv_obj_align(passLabel, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_text_font(passLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(passLabel, lv_color_hex(0x888888), 0);

    Serial.println("[QRGenerator] ✓ Setup screen created");
}

// ========================================
// Private Helper Methods
// ========================================

lv_obj_t* QRGenerator::drawQROnCanvas(lv_obj_t* screen,
                                        QRCode& qrcode,
                                        int16_t x,
                                        int16_t y,
                                        uint8_t scale) {
    // Calculate canvas size (add border)
    const uint8_t border = 2;
    uint16_t qr_size = qrcode.size;
    uint16_t canvas_size = (qr_size + 2 * border) * scale;

    // Create canvas
    lv_obj_t* canvas = lv_canvas_create(screen);
    lv_obj_set_size(canvas, canvas_size, canvas_size);

    // Allocate canvas buffer
    static lv_color_t* cbuf = nullptr;
    if (cbuf) {
        heap_caps_free(cbuf);
    }

    cbuf = (lv_color_t*)heap_caps_malloc(
        LV_CANVAS_BUF_SIZE_TRUE_COLOR(canvas_size, canvas_size),
        MALLOC_CAP_SPIRAM
    );

    if (!cbuf) {
        Serial.println("[QRGenerator] ✗ Failed to allocate canvas buffer");
        return nullptr;
    }

    lv_canvas_set_buffer(canvas, cbuf, canvas_size, canvas_size, LV_IMG_CF_TRUE_COLOR);

    // Fill white background
    lv_canvas_fill_bg(canvas, lv_color_hex(0xFFFFFF), LV_OPA_COVER);

    // Draw QR code
    for (uint8_t y_pos = 0; y_pos < qr_size; y_pos++) {
        for (uint8_t x_pos = 0; x_pos < qr_size; x_pos++) {
            if (qrcode_getModule(&qrcode, x_pos, y_pos)) {
                // Black module
                lv_canvas_set_px(canvas,
                                (x_pos + border) * scale,
                                (y_pos + border) * scale,
                                lv_color_hex(0x000000));

                // Scale up (fill block)
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        lv_canvas_set_px(canvas,
                                        (x_pos + border) * scale + sx,
                                        (y_pos + border) * scale + sy,
                                        lv_color_hex(0x000000));
                    }
                }
            }
        }
    }

    // Position canvas (-1 means center)
    if (x == -1 && y == -1) {
        lv_obj_center(canvas);
    } else if (x == -1) {
        lv_obj_align(canvas, LV_ALIGN_TOP_MID, 0, y);
    } else if (y == -1) {
        lv_obj_align(canvas, LV_ALIGN_LEFT_MID, x, 0);
    } else {
        lv_obj_set_pos(canvas, x, y);
    }

    return canvas;
}

String QRGenerator::generateWiFiQRString(const String& ssid,
                                          const String& password,
                                          const String& security) {
    // WiFi QR format: WIFI:S:ssid;T:security;P:password;;
    // Security types: WPA, WEP, nopass

    String qrString = "WIFI:S:" + ssid + ";";

    if (!password.isEmpty()) {
        qrString += "T:" + security + ";P:" + password + ";";
    } else {
        qrString += "T:nopass;";
    }

    qrString += ";";

    return qrString;
}

} // namespace Doki
