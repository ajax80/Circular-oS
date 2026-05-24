#ifndef TIMER_H
#define TIMER_H

typedef unsigned int uint32_t;

void     timer_init(void);
uint32_t timer_poll(void);   /* call frequently — returns ms tick count */
uint32_t timer_ticks(void);

#endif
