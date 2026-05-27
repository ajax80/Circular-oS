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

#include "mouse.h"

int     mouse_x = 400;
int     mouse_y = 300;
uint8_t mouse_buttons = 0;

static int bounds_w = 800;
static int bounds_h = 600;

static void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}
static unsigned char inb(unsigned short port) {
    unsigned char v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static void ps2_wait_in(void) {
    unsigned int t = 100000;
    while (t-- && (inb(0x64) & 2));
}
static void ps2_wait_out(void) {
    unsigned int t = 100000;
    while (t-- && !(inb(0x64) & 1));
}
static void mouse_send(unsigned char b) {
    ps2_wait_in(); outb(0x64, 0xD4);
    ps2_wait_in(); outb(0x60, b);
}
static unsigned char mouse_recv(void) {
    ps2_wait_out();
    return inb(0x60);
}

void mouse_set_bounds(int w, int h) {
    bounds_w = w; bounds_h = h;
    mouse_x  = w / 2;
    mouse_y  = h / 2;
}

void mouse_init(void) {
    /* enable aux device */
    ps2_wait_in(); outb(0x64, 0xA8);
    /* read + patch command byte: enable IRQ12, clear mouse-disable */
    ps2_wait_in(); outb(0x64, 0x20);
    ps2_wait_out();
    unsigned char cb = (inb(0x60) | 0x02) & ~0x20u;
    ps2_wait_in(); outb(0x64, 0x60);
    ps2_wait_in(); outb(0x60, cb);
    /* enable data reporting */
    mouse_send(0xF4);
    mouse_recv(); /* ACK */
}

static unsigned char pkt[3];
static int           cycle = 0;

void mouse_poll(void) {
    /* bit 0 = data ready, bit 5 = from mouse */
    while ((inb(0x64) & 0x21) == 0x21) {
        unsigned char b = inb(0x60);
        if (cycle == 0 && !(b & 0x08)) continue; /* sync: bit 3 always set */
        pkt[cycle++] = b;
        if (cycle < 3) continue;
        cycle = 0;

        mouse_buttons = pkt[0] & 0x07;
        int dx = (int)(signed char)pkt[1];
        int dy = (int)(signed char)pkt[2];
        mouse_x += dx;
        mouse_y -= dy; /* PS/2 Y positive = up; screen Y positive = down */
        if (mouse_x < 0)           mouse_x = 0;
        if (mouse_y < 0)           mouse_y = 0;
        if (mouse_x >= bounds_w)   mouse_x = bounds_w - 1;
        if (mouse_y >= bounds_h)   mouse_y = bounds_h - 1;
    }
}
