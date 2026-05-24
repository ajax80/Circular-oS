#ifndef VGA_H
#define VGA_H

void vga_set_buffer(unsigned int addr);
void vga_init(void);
void vga_putc(char c);
void vga_puts(const char *s);
void vga_putu(unsigned int v);
void vga_puth(unsigned int v);

#endif
