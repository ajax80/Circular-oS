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

#include "serial.h"
#include "vga.h"
#include "fb.h"

#define COM1 0x3F8

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void serial_putc(char c) {
    vga_putc(c);
    fb_putc(c);
    while (!(inb(COM1 + 5) & 0x20));
    outb(COM1, c);
}

void serial_puts(const char *s) {
    while (*s) serial_putc(*s++);
}

void serial_puth(unsigned int v) {
    const char *h = "0123456789abcdef";
    int i;
    serial_puts("0x");
    for (i = 28; i >= 0; i -= 4)
        serial_putc(h[(v >> i) & 0xF]);
}

static const char sc_map[58] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0,  '*', 0,  ' '
};

/* returns char or 0 if no key ready (non-blocking) */
int serial_trygetc(void) {
    unsigned char st = inb(0x64);
    if (!(st & 0x01)) return 0;   /* no data */
    if (st & 0x20)    return 0;   /* mouse data, not keyboard */
    unsigned char sc = inb(0x60);
    if (sc & 0x80) return 0;      /* key release */
    if (sc < sizeof(sc_map) && sc_map[sc]) return (int)(unsigned char)sc_map[sc];
    return 0;
}

char serial_getc(void) {
    int c;
    while (!(c = serial_trygetc()));
    return (char)c;
}

void serial_gets(char *buf, int max) {
    int i = 0;
    while (i < max - 1) {
        char c = serial_getc();
        if (c == '\r' || c == '\n') { serial_puts("\r\n"); break; }
        if (c == '\b' || c == 127) {
            if (i > 0) { i--; serial_puts("\b \b"); }
            continue;
        }
        buf[i++] = c;
        serial_putc(c);
    }
    buf[i] = '\0';
}

void serial_putu(unsigned int v) {
    char buf[12];
    int i = 0, j;
    if (!v) { serial_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    for (j = i - 1; j >= 0; j--) serial_putc(buf[j]);
}
