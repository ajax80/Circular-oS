#include "vga.h"

#define VGA_BUF  ((unsigned short *)0xB8000)
#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_ATTR 0x0F

static unsigned short *vga = VGA_BUF;

void vga_set_buffer(unsigned int addr) {
    vga = (unsigned short *)addr;
}
static int col = 0, row = 0;

static void scroll(void) {
    int i;
    for (i = 0; i < VGA_COLS * (VGA_ROWS - 1); i++)
        vga[i] = vga[i + VGA_COLS];
    for (i = VGA_COLS * (VGA_ROWS - 1); i < VGA_COLS * VGA_ROWS; i++)
        vga[i] = (VGA_ATTR << 8) | ' ';
    row = VGA_ROWS - 1;
}

void vga_init(void) {
    int i;
    col = row = 0;
    for (i = 0; i < VGA_COLS * VGA_ROWS; i++)
        vga[i] = (VGA_ATTR << 8) | ' ';
}

void vga_putc(char c) {
    if (c == '\n') { col = 0; row++; }
    else if (c == '\r') { col = 0; }
    else if (c == '\b') { if (col > 0) col--; }
    else {
        vga[row * VGA_COLS + col] = (VGA_ATTR << 8) | (unsigned char)c;
        if (++col >= VGA_COLS) { col = 0; row++; }
    }
    if (row >= VGA_ROWS) scroll();
}

void vga_puts(const char *s) {
    while (*s) vga_putc(*s++);
}

void vga_putu(unsigned int v) {
    char buf[12];
    int i = 0, j;
    if (!v) { vga_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    for (j = i - 1; j >= 0; j--) vga_putc(buf[j]);
}

void vga_puth(unsigned int v) {
    const char *h = "0123456789abcdef";
    int i;
    vga_puts("0x");
    for (i = 28; i >= 0; i -= 4)
        vga_putc(h[(v >> i) & 0xF]);
}
