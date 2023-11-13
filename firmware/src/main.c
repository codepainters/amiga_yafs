// SPDX-FileCopyrightText: 2023 Przemysław Węgrzyn <pwegrzyn@codepainters.com>
// SPDX-License-Identifier: MIT

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdbool.h>

#include "config.h"
#include "timer.h"

#define ENABLE_PULLUP(port, pin) \
    do { *((uint8_t *) &(port) + 0x10 + pin) |= PORT_PULLUPEN_bm; } while(0)

#define DISABLE_PULLUP(port, pin) \
    do { *((uint8_t *) &(port) + 0x10 + pin) &= ~PORT_PULLUPEN_bm; } while(0)

static bool cfg__beep_on_boot;
static bool cfg__save_last_mode;
static bool cfg__swap_on_start;

static volatile bool switch_pending = false;
static bool drives_swapped;

static volatile int16_t nrst_cnt = NRST_PRESS_DURATION;

#define EEPROM_SWAP_MARKER (0xA5)

static inline void cfg__init() {
    // note: pins are inputs by default, no direction setting necessary, pull-ups are
    // already enabled, too.

    // default (pin open) is no beep, shorted to ground - beep on boot
    cfg__beep_on_boot = (IN_CFG_PORT.IN & (1 << IN_CFG1_PIN)) == 0;
    // default is to save/restore last mode
    cfg__save_last_mode = (IN_CFG_PORT.IN & (1 << IN_CFG2_PIN)) != 0;
    // default is to swap on start (only relevant if cfg__save_last_mode is false)
    cfg__swap_on_start = (IN_CFG_PORT.IN & (1 << IN_CFG3_PIN)) != 0;
}

static void routing__init() {
    OUT_SEL0_PORT.OUTSET = OUT_SEL0_bm;
    OUT_SEL0_PORT.DIRSET = OUT_SEL0_bm;
    OUT_SEL1_PORT.OUTSET = OUT_SEL1_bm;
    OUT_SEL1_PORT.DIRSET = OUT_SEL1_bm;

    OUT_MODE_PORT.DIRSET = OUT_MODE_bm;

    // set PA2/PB2 as EVOUTs
    PORTMUX.CTRLA = 0x03;
    EVSYS.ASYNCCH0 = IN_SEL1_EV;
    EVSYS.ASYNCCH1 = IN_SEL0_EV;
}

static void routing__set(bool swapped) {
    if (swapped) {
        OUT_MODE_PORT.OUT = OUT_MODE_bm;
        // EVOUT0 (PA2) <- ASYNCCH1
        EVSYS.ASYNCUSER8 = EVSYS_ASYNCUSER8_ASYNCCH1_gc;
        // EVOUT1 (PB2) <- ASYNCCH0
        EVSYS.ASYNCUSER9 = EVSYS_ASYNCUSER9_ASYNCCH0_gc;
    } else {
        OUT_MODE_PORT.OUTCLR = OUT_MODE_bm;
        EVSYS.ASYNCUSER8 = EVSYS_ASYNCUSER8_ASYNCCH0_gc;
        EVSYS.ASYNCUSER9 = EVSYS_ASYNCUSER9_ASYNCCH1_gc;
    }
}

uint8_t eeprom__byte_read(uint16_t addr) {
    // hardware stalls execution if EEPROM is busy
    return *(uint8_t *) (EEPROM_START + addr);
}

void eeprom__byte_write(uint16_t addr, uint8_t data) {
    // previous write has to complete
    while (NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);

    // clear bufer, set byte, erase & write buffer back
    _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEBUFCLR_gc);
    *(uint8_t *) (EEPROM_START + addr) = data;
    _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
}

static void switch_config() {
    drives_swapped = !drives_swapped;
    if (cfg__save_last_mode) {
        eeprom__byte_write(0, drives_swapped ? EEPROM_SWAP_MARKER : 0);
    }

    routing__set(drives_swapped);
    timer__beep(drives_swapped);
}

int main(void) {
    // enable pull-ups on all pins by default
    for (uint8_t i = 0; i < 8; i++) {
        ENABLE_PULLUP(PORTA, i);
        ENABLE_PULLUP(PORTB, i);
    }

    cfg__init();
    routing__init();

    // these signals are externally pulled up
    DISABLE_PULLUP(IN_NRST_PORT, IN_NRST_PIN);
    DISABLE_PULLUP(IN_SEL0_PORT, IN_SEL0_PIN);
    DISABLE_PULLUP(IN_SEL1_PORT, IN_SEL1_PIN);

    // configure main clock - internal 20MHz / 16 -> 1.25MHz, lock configuration
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_16X_gc | CLKCTRL_PEN_bm);
    _PROTECTED_WRITE(CLKCTRL.MCLKLOCK, CLKCTRL_LOCKEN_bm);

    timer__init();

    if (cfg__save_last_mode) {
        drives_swapped = eeprom__byte_read(0) == EEPROM_SWAP_MARKER;
    } else {
        drives_swapped = cfg__swap_on_start;
    }
    routing__set(drives_swapped);

    SLPCTRL.CTRLA = SLPCTRL_SEN_bm | SLPCTRL_SMODE_IDLE_gc;
    sei();

    if (cfg__beep_on_boot) {
        timer__delay(600);
        timer__beep(drives_swapped);
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        sleep_cpu();
        if (switch_pending) {
            switch_config();
            switch_pending = false;
        }
    }
#pragma clang diagnostic pop
}

void timer__tick() {
    bool nrst_active = (IN_NRST_PORT.IN & IN_NRST_bm) == 0;
    if (nrst_active) {
        if (nrst_cnt == 1) {
            switch_pending = true;
            nrst_cnt = 0;
        }
        if (nrst_cnt > 0) {
            nrst_cnt--;
        }
    } else {
        nrst_cnt = NRST_PRESS_DURATION;
    }
}
