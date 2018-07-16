#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define VENDOR_NAME  "Sakura Industries Limited"
#define INDEX_URL "http://www.sakoora.com/"
#define PRODUCT_NAME "ESP32-MAXIO"
#define VOLUME_LABEL "ESP32-MAXIO"

#define BOARD_ID "SAML21E18B-Esp32-MAXIO"

//#define USB_VID 0x2341
//#define USB_PID 0x024D

// Cause bootloader to detect if running under a SW Debugger
// And pass that information to the App.
// This ALSO defines the code value passed to the application.
#define DETECT_DEBUGGER_M0 0xAFFEC7ED

// No Led
//#define LED_PIN PIN_PA17

// String of 4 x WS2812B RGB Leds.
#define BOARD_NEOPIXEL_PIN PIN_PA06
#define BOARD_NEOPIXEL_COUNT 4

// This target has a RESET BUTTON, so double tap reset is used to get into bootloader.

#endif
