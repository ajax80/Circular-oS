#include "idt.h"

typedef struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t   idtp;

void idt_set_gate(uint8_t n, uint32_t handler) {
    idt[n].base_lo = (uint16_t)(handler & 0xFFFF);
    idt[n].sel     = 0x08;
    idt[n].zero    = 0;
    idt[n].flags   = 0x8E;
    idt[n].base_hi = (uint16_t)(handler >> 16);
}

void idt_init(void) {
    unsigned int i;
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    for (i = 0; i < 256; i++) {
        idt[i].base_lo = 0; idt[i].sel = 0;
        idt[i].zero    = 0; idt[i].flags = 0;
        idt[i].base_hi = 0;
    }
    __asm__ volatile("lidt %0" :: "m"(idtp));
}
