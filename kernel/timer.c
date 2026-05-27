/*
 * Circular OS
 * Copyright (C) 2026 Jonathan Eugene Ayers <ayersjon80@gmail.com>
 *
 * This file is part of Circular OS.
 *
 * Circular OS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Circular OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with Circular OS. If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * For commercial licensing, see COMMERCIAL_LICENSE in this repository.
 */

#include "timer.h"

static unsigned int pit_ticks  = 0;
static unsigned short pit_last = 0xFFFF;

static void outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static unsigned char inb(unsigned short p) {
    unsigned char v;
    __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));
    return v;
}

void timer_init(void) {
    /* PIT channel 0, mode 3, 1000 Hz */
    unsigned short div = 1193;
    outb(0x43, 0x36);
    outb(0x40, (unsigned char)(div & 0xFF));
    outb(0x40, (unsigned char)(div >> 8));
    pit_last = 0xFFFF;
    pit_ticks = 0;
}

uint32_t timer_poll(void) {
    /* latch channel 0 and read current count */
    outb(0x43, 0x00);
    unsigned char lo = inb(0x40);
    unsigned char hi = inb(0x40);
    unsigned short now = (unsigned short)((unsigned short)hi << 8 | lo);
    /* counter counts down; if now > last it wrapped = 1ms elapsed */
    if (now > pit_last) pit_ticks++;
    pit_last = now;
    return pit_ticks;
}

uint32_t timer_ticks(void) { return pit_ticks; }
