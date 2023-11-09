#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <avr/io.h>

// All configuration pins use PORTA.
#define IN_CFG_PORT    (PORTA)
#define IN_CFG1_PIN    (5)
#define IN_CFG2_PIN    (6)
#define IN_CFG3_PIN    (7)

#define IN_SEL0_PORT   (PORTB)
#define IN_SEL0_PIN    (0)
#define IN_SEL1_PORT   (PORTA)
#define IN_SEL1_PIN    (1)

#define OUT_SEL0_PORT   (PORTB)
#define OUT_SEL0_PIN    (2)
#define OUT_SEL0_bm     (1 << OUT_SEL0_PIN)
#define OUT_SEL1_PORT   (PORTA)
#define OUT_SEL1_PIN    (2)
#define OUT_SEL1_bm     (1 << OUT_SEL1_PIN)

#define OUT_BZR_PORT    (PORTB)
#define OUT_BZR_PIN     (1)
#define OUT_BZR_bm      (1 << OUT_BZR_PIN)

#define IN_NRST_PORT    (PORTB)
#define IN_NRST_PIN     (3)
#define IN_NRST_bm      (1 << IN_NRST_PIN)

#define OUT_MODE_PORT   (PORTA)
#define OUT_MODE_PIN    (4)
#define OUT_MODE_bm     (1 << OUT_MODE_PIN)

// 20Mhz / 16 / 625 -> 2kHz
#define TCA0_PERIOD     (625)


#define NRST_PRESS_DURATION (2000)

#endif