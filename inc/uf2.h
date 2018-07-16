#ifndef UF2_H
#define UF2_H 1

#include "board_config.h"

#include "sam.h"
#define UF2_DEFINE_HANDOVER 1 // for testing
#include "uf2format.h"
#include "uf2hid.h"
#include "main.h"
#include "cdc_enumerate.h"
#include "sam_ba_monitor.h"
#include "usart_sam_ba.h"
#include <stdio.h>
#include <string.h>

#undef DISABLE
#undef ENABLE

// always go for crystalless - smaller and more compatible
#ifndef CRYSTALLESS
#define CRYSTALLESS 1
#endif

#ifndef USB_PID
#define USB_VID 0x03EB // Atmel
#define USB_PID 0x2402 // Generic HID device
#endif

#ifndef INDEX_URL
#define INDEX_URL "https://www.pxt.io/"
#endif

#include "uf2_version.h"

// needs to be more than ~4200 (to force FAT16)
#define NUM_FAT_BLOCKS 8000

// Logging to help debugging
#define USE_LOGS 0
// Check various conditions; best leave on
#define USE_ASSERT 0 // 188 bytes
// Enable reading flash via FAT files; otherwise drive will appear empty
#define USE_FAT 1 // 272 bytes
// Enable index.htm file on the drive
#define USE_INDEX_HTM 1 // 132 bytes
// Enable USB CDC (Communication Device Class; i.e., USB serial) monitor for Arduino style flashing
#define USE_CDC 1 // 1264 bytes (plus terminal, see below)
// Support the UART (real serial port, not USB)
#define USE_UART 0
// Support Human Interface Device (HID) - serial, flashing and debug
#define USE_HID 1 // 788 bytes
// Expose HID via WebUSB
#define USE_WEBUSB 1
// Doesn't yet disable code, just enumeration
#define USE_MSC 1

// If enabled, bootloader will start on power-on and every reset. A second reset
// will start the app. This only happens if the app says it wants that (see SINGLE_RESET() below).
// If disabled here or by the app, the bootloader will only start with double-click of the reset
// button.
#define USE_SINGLE_RESET 1

// Fine-tuning of features
#define USE_HID_SERIAL 0   // just an example, not really needed; 36 bytes
#define USE_HID_EXT 1      // extended HID commands (read/write mem); 60 bytes
#define USE_HID_HANDOVER 1 // allow HID application->bootloader seamless transition; 56 bytes
#define USE_MSC_HANDOVER 1 // ditto for MSC; 348 bytes
#define USE_MSC_CHECKS 0   // check validity of MSC commands; 460 bytes
#define USE_CDC_TERMINAL 0 // enable ASCII mode on CDC loop (not used by BOSSA); 228 bytes
#define USE_DBG_MSC 0      // output debug info about MSC

#if USE_CDC
#define CDC_VERSION "S"
#else
#define CDC_VERSION ""
#endif

#if USE_LOGS
#define LOGS_VERSION "L"
#else
#define LOGS_VERSION ""
#endif

#if USE_FAT
#define FAT_VERSION "F"
#else
#define FAT_VERSION ""
#endif

#if USE_ASSERT
#define ASSERT_VERSION "A"
#else
#define ASSERT_VERSION ""
#endif

#if USE_HID
#define HID_VERSION "H"
#else
#define HID_VERSION ""
#endif

#if USE_SINGLE_RESET
#define RESET_VERSION "R"
#else
#define RESET_VERSION ""
#endif

#if USE_WEBUSB
#define WEB_VERSION "W"
#else
#define WEB_VERSION ""
#endif

#if USE_MSC_HANDOVER
#define MSC_HANDOVER_VERSION "O"
#else
#define MSC_HANDOVER_VERSION ""
#endif

#define UF2_VERSION                                                                                \
    UF2_VERSION_BASE " " CDC_VERSION LOGS_VERSION FAT_VERSION ASSERT_VERSION HID_VERSION           \
        WEB_VERSION RESET_VERSION MSC_HANDOVER_VERSION

// End of config

#define USE_MONITOR (USE_CDC || USE_UART)
#define TIMER_STEP 1500



/*
From CPU config:
#define FLASH_SIZE            0x8000UL
#define FLASH_PAGE_SIZE       64
#define FLASH_NB_OF_PAGES     512
*/

// These two need to be defined as plain decimal numbers, as we're using # on them
#define FLASH_ROW_SIZE 256
#ifndef FLASH_NUM_ROWS
#define FLASH_NUM_ROWS 1024
#endif

#define NOOP                                                                                       \
    do {                                                                                           \
    } while (0)

#if USE_LOGS
struct LogStore {
    int ptr;
    char buffer[4096];
};
extern struct LogStore logStoreUF2;
void logmsg(const char *msg);
void logval(const char *lbl, uint32_t v);
void logwritenum(uint32_t n);
void logwrite(const char *msg);
void logreset(void);
#else
#define logmsg(...) NOOP
#define logval(...) NOOP
#define logwritenum(...) NOOP
#define logwrite(...) NOOP
#define logreset() NOOP
#endif

#if USE_DBG_MSC
#define DBG_MSC(x) x
#else
#define DBG_MSC(x) NOOP
#endif

void panic(int code);

#if USE_ASSERT
#define assert(cond)                                                                               \
    if (!(cond)) {                                                                                 \
        panic(__LINE__);                                                                           \
    }
#else
#define assert(cond) NOOP
#endif

extern volatile bool b_sam_ba_interface_usart;
void flash_write_row(uint32_t *dst, uint32_t *src);
void flash_erase_to_end(uint32_t *start_address);
void flash_write_words(uint32_t *dst, uint32_t *src, uint32_t n_words);
void copy_words(uint32_t *dst, uint32_t *src, uint32_t n_words);

int writeNum(char *buf, uint32_t n, bool full);

void process_hid(void);

// index of highest LUN
#define MAX_LUN 0
void process_msc(void);
void msc_reset(void);
//! Static block size for all memories
#define UDI_MSC_BLOCK_SIZE 512L

void read_block(uint32_t block_no, uint8_t *data);
#define MAX_BLOCKS (FLASH_SIZE / 256 + 100)
typedef struct {
    uint32_t numBlocks;
    uint32_t numWritten;
    uint8_t writtenMask[MAX_BLOCKS / 8 + 1];
} WriteState;
void write_block(uint32_t block_no, uint8_t *data, bool quiet, WriteState *state);
void padded_memcpy(char *dst, const char *src, int len);

// Last word in RAM
// Unlike for ordinary applications, our link script doesn't place the stack at the bottom
// of the RAM, but instead after all allocated BSS.
// In other words, this word should survive reset.
#ifdef SAMD21
#define DBL_TAP_PTR ((volatile uint32_t *)(HMCRAMC0_ADDR + HMCRAMC0_SIZE - 4))
#endif
#ifdef SAML21
#define DBL_TAP_PTR ((volatile uint32_t *)(HSRAM_ADDR + HSRAM_SIZE - 4))
#endif
#ifdef SAMD51
#define DBL_TAP_PTR ((volatile uint32_t *)(HSRAM_ADDR + HSRAM_SIZE - 4))
#endif
#define DBL_TAP_MAGIC 0xf01669ef // Randomly selected, adjusted to have first and last bit set
#define DBL_TAP_MAGIC_QUICK_BOOT 0xf02669ef

#if USE_SINGLE_RESET
#define SINGLE_RESET() (*((uint32_t *)0x20B4) == 0x87eeb07c)
#endif

void resetIntoApp(void);
void resetIntoBootloader(void);
void system_init(void);

#define PINOP(pin, OP) (PORT->Group[(pin) / 32].OP.reg = (1 << ((pin) % 32)))
#define PINIP(pin) (((PORT->Group[(pin) / 32].IN.reg) >> ((pin) % 32)) & 0x1)
#define PINCFG(pin) (PORT->Group[(pin) / 32].PINCFG[(pin) % 32].reg)
#define PINMUX(pin) (PORT->Group[(pin) / 32].PMUX[((pin) % 32)/2].reg)

typedef enum {
  SIGNAL_HB      = 0,
  SIGNAL_START,
  SIGNAL_USB,
#if USE_UART
  SIGNAL_UART,
#endif  
  SIGNAL_LEAVE,
  SIGNAL_FLASHWR,
  SIGNAL_MAX    // Not a signal, used to size the enum. ALWAYS LAST.
} SigState;

void signal_init(void);
void signal_tick(void);
void signal_state(SigState state);
//void signal_state(uint8_t state);
/*
#define SIGNAL_HB      0x00
#define SIGNAL_START   0x01
#define SIGNAL_USB     0x02
#define SIGNAL_UART    0x03
#define SIGNAL_LEAVE   0x04
#define SIGNAL_FLASHWR 0x05
*/

extern uint32_t timerHigh, resetHorizon;
void timerTick(void);
void delay(uint32_t ms);
void hidHandoverLoop(int ep);
void handoverPrep(void);

#define CONCAT_1(a, b) a##b
#define CONCAT_0(a, b) CONCAT_1(a, b)
#define STATIC_ASSERT(e) enum { CONCAT_0(_static_assert_, __LINE__) = 1 / ((e) ? 1 : 0) }

#ifdef SAMD21
STATIC_ASSERT(FLASH_ROW_SIZE == FLASH_PAGE_SIZE * 4);
STATIC_ASSERT(FLASH_ROW_SIZE == NVMCTRL_ROW_SIZE);
STATIC_ASSERT(FLASH_NUM_ROWS * 4 == FLASH_NB_OF_PAGES);
#endif

extern const char infoUf2File[];

#endif
