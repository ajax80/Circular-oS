#ifndef SERIAL_H
#define SERIAL_H

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
void serial_putu(unsigned int v);
void serial_puth(unsigned int v);
int  serial_trygetc(void);
char serial_getc(void);
void serial_gets(char *buf, int max);

#endif
