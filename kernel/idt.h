#ifndef IDT_H
#define IDT_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

void idt_init(void);
void idt_set_gate(uint8_t n, uint32_t handler);

#endif
