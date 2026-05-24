#ifndef TIMER_H
#define TIMER_H

typedef unsigned int uint32_t;

void     timer_init(void);
uint32_t timer_ticks(void);
void     timer_wait_ms(uint32_t ms);

/* called from IRQ0 handler */
void irq0_handler(void);

#endif
