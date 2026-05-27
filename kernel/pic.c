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

#include "pic.h"

static void outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static unsigned char inb(unsigned short p) {
    unsigned char v;
    __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));
    return v;
}
static void io_wait(void) { outb(0x80, 0); }

void pic_init(void) {
    /* save masks */
    unsigned char m1 = inb(0x21), m2 = inb(0xA1);

    /* ICW1: cascade mode, ICW4 needed */
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    /* ICW2: vector offsets */
    outb(0x21, 0x20); io_wait();   /* master IRQ0-7 → INT 0x20-0x27 */
    outb(0xA1, 0x28); io_wait();   /* slave  IRQ8-15 → INT 0x28-0x2F */
    /* ICW3 */
    outb(0x21, 0x04); io_wait();   /* master: slave on IRQ2 */
    outb(0xA1, 0x02); io_wait();   /* slave: cascade identity 2 */
    /* ICW4: 8086 mode */
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    /* restore masks — mask everything except IRQ0 (timer) */
    outb(0x21, (m1 & 0xFF) | 0xFE);   /* keep all masked except bit 0 */
    outb(0xA1, m2 | 0xFF);
    (void)m1; (void)m2;
    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void pic_unmask(uint8_t irq) {
    unsigned short port = (irq < 8) ? 0x21 : 0xA1;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) & ~(1u << irq));
}

void pic_mask(uint8_t irq) {
    unsigned short port = (irq < 8) ? 0x21 : 0xA1;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) | (1u << irq));
}
