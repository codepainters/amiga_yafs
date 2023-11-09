// SPDX-FileCopyrightText: 2023 Przemysław Węgrzyn <pwegrzyn@codepainters.com>
// SPDX-License-Identifier: MIT

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdbool.h>

void timer__init();

void timer__delay(int delay_ticks);

void timer__beep(bool twice);

void timer__tick();

#endif