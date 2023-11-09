#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "config.h"

#define ENABLE_PULLUP(port, pin) \
    do { *((uint8_t *) &(port) + 0x10 + pin) |= PORT_PULLUPEN_bm; } while(0)

static bool cfg__beep_on_boot;
static bool cfg__save_last_mode;
static bool cfg__swap_on_start;

static volatile bool switch_pending = false;
static bool drives_swapped;

static volatile int16_t delay_cnt = 0;
static volatile int16_t nrst_cnt = NRST_PRESS_DURATION;

#define EEPROM_SWAP_MARKER (0xA5)

static void io_init() {
    // enable pull-ups on all pins by default
    for (uint8_t i = 0; i < 8; i++) {
        ENABLE_PULLUP(PORTA, i);
        ENABLE_PULLUP(PORTB, i);
    }

    // buzzer output should idle at 0
    OUT_BZR_PORT.OUTCLR = OUT_BZR_bm;
    OUT_BZR_PORT.DIRSET = OUT_BZR_bm;

    OUT_SEL0_PORT.OUTSET = OUT_SEL0_bm;
    OUT_SEL0_PORT.DIRSET = OUT_SEL0_bm;
    OUT_SEL1_PORT.OUTSET = OUT_SEL1_bm;
    OUT_SEL1_PORT.DIRSET = OUT_SEL1_bm;

    OUT_MODE_PORT.DIRSET = OUT_MODE_bm;

    // note: pins are inputs by default, no direction setting necessary.
    ENABLE_PULLUP(IN_CFG_PORT, IN_CFG1_PIN);
    ENABLE_PULLUP(IN_CFG_PORT, IN_CFG2_PIN);
    ENABLE_PULLUP(IN_CFG_PORT, IN_CFG3_PIN);

    ENABLE_PULLUP(IN_NRST_PORT, IN_NRST_PIN);

    // NRST, SEL0_IN, SEL1_IN use default settings

    // TODO: disable unnecessary pull-ups!
}

static void delay(int delay_ticks) {
    delay_cnt = delay_ticks;
    while (delay_cnt);
}

static void set_routing(bool swapped) {
    if (swapped) {
        OUT_MODE_PORT.OUT = OUT_MODE_bm;
        // EVOUT0 (PA2) <- ASYNCCH1
        EVSYS.ASYNCUSER8 = 0x04;
        // EVOUT1 (PB2) <- ASYNCCH0
        EVSYS.ASYNCUSER9 = 0x03;
    } else {
        OUT_MODE_PORT.OUTCLR = OUT_MODE_bm;
        EVSYS.ASYNCUSER8 = 0x03;
        EVSYS.ASYNCUSER9 = 0x04;
    }
}

static inline void enable_buzzer(bool enable) {
    if (enable) {
        TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    } else {
        TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    }
}

static void beep(bool twice) {
    enable_buzzer(true);
    delay(200);
    enable_buzzer(false);

    if (twice) {
        delay(200);
        enable_buzzer(true);
        delay(200);
        enable_buzzer(false);
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

    set_routing(drives_swapped);
    beep(drives_swapped);
}

int main(void) {
    io_init();

    // configure main clock - internal 20MHz / 16 -> 1.25MHz, lock configuration
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_16X_gc | CLKCTRL_PEN_bm);
    _PROTECTED_WRITE(CLKCTRL.MCLKLOCK, CLKCTRL_LOCKEN_bm);

    // TCA0 is used toe generate 2kHz buzzer tone and measure delays (via overflow interrupt)
    TCA0.SINGLE.PER = TCA0_PERIOD;
    TCA0.SINGLE.CMP1 = TCA0_PERIOD / 2;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;

    enable_buzzer(false);

    // TODO: review
    PORTMUX.CTRLA = 0x03;
    EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_PORTA_PIN1_gc;
    EVSYS.ASYNCCH1 = EVSYS_ASYNCCH1_PORTB_PIN0_gc;

    // NVMCTRL.CTRLB = 0 << NVMCTRL_APCWP_bp /* Application code write protect: disabled */
    //		 | 0 << NVMCTRL_BOOTLOCK_bp; /* Boot Lock: disabled */
    // NVMCTRL.INTCTRL = 0 << NVMCTRL_EEREADY_bp; /* EEPROM Ready: disabled */


    cfg__beep_on_boot = (IN_CFG_PORT.IN & (1 << IN_CFG1_PIN)) == 0;
    cfg__save_last_mode = (IN_CFG_PORT.IN & (1 << IN_CFG2_PIN)) != 0;
    cfg__swap_on_start = (IN_CFG_PORT.IN & (1 << IN_CFG3_PIN)) != 0;

    if (cfg__save_last_mode) {
        drives_swapped = eeprom__byte_read(0) == EEPROM_SWAP_MARKER;
    } else {
        drives_swapped = cfg__swap_on_start;
    }

    set_routing(drives_swapped);

    SLPCTRL.CTRLA = SLPCTRL_SEN_bm | SLPCTRL_SMODE_IDLE_gc;

    sei();
#if 1
    if (cfg__beep_on_boot) {
        delay(600);
        beep(drives_swapped);
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        // sleep();
        if (switch_pending) {
            switch_config();
            switch_pending = false;
        }
    }
#pragma clang diagnostic pop
#endif

}

ISR(TCA0_OVF_vect) {
    if (delay_cnt > 0) {
        delay_cnt--;
    }

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

    /* The interrupt flag has to be cleared manually */
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}
