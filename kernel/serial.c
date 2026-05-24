#include "serial.h"

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

char serial_getc(void) {
    while (!(inb(COM1 + 5) & 0x01));
    return (char)inb(COM1);
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
