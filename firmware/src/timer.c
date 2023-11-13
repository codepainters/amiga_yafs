// SPDX-FileCopyrightText: 2023 Przemysław Węgrzyn <pwegrzyn@codepainters.com>
// SPDX-License-Identifier: MIT

#include "timer.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>

static volatile int16_t delay_cnt = 0;

void timer__init() {
    // buzzer output should idle at 0
    OUT_BZR_PORT.OUTCLR = OUT_BZR_bm;
    OUT_BZR_PORT.DIRSET = OUT_BZR_bm;

    // TCA0 is used to generate 2kHz buzzer tone and measure delays (via overflow interrupt)
    TCA0.SINGLE.PER = TCA0_PERIOD;
    TCA0.SINGLE.CMP1 = TCA0_PERIOD / 2;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;
}

void timer__delay(int delay_ticks) {
    delay_cnt = delay_ticks;
    while (delay_cnt);
}

static inline void timer__enable_buzzer(bool enable) {
    TCA0.SINGLE.CTRLB = (enable ? TCA_SINGLE_CMP1EN_bm : 0) | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
}

void timer__beep(bool twice) {
    timer__enable_buzzer(true);
    timer__delay(BEEP_DURATION);
    timer__enable_buzzer(false);

    if (twice) {
        timer__delay(BEEP_DURATION);
        timer__enable_buzzer(true);
        timer__delay(BEEP_DURATION);
        timer__enable_buzzer(false);
    }
}

ISR(TCA0_OVF_vect) {
    if (delay_cnt > 0) {
        delay_cnt--;
    }
    timer__tick();

    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}
