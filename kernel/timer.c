#include "timer.h"
#include "audio.h"

static volatile unsigned int ticks = 0;

static void outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}

void timer_init(void) {
    /* PIT channel 0, mode 3 (square wave), 1000 Hz */
    unsigned short div = 1193;
    outb(0x43, 0x36);
    outb(0x40, (unsigned char)(div & 0xFF));
    outb(0x40, (unsigned char)(div >> 8));
}

uint32_t timer_ticks(void) { return ticks; }

void timer_wait_ms(uint32_t ms) {
    uint32_t end = ticks + ms;
    while (ticks < end) __asm__ volatile("hlt");
}

void irq0_handler(void) {
    ticks++;
    audio_tick(ticks);
    /* EOI to master PIC */
    __asm__ volatile("outb %0,%1"::"a"((unsigned char)0x20),"Nd"((unsigned short)0x20));
}
