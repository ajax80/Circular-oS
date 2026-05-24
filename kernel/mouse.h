#ifndef MOUSE_H
#define MOUSE_H

typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;

void    mouse_init(void);
void    mouse_poll(void);
void    mouse_set_bounds(int w, int h);

extern int    mouse_x;
extern int    mouse_y;
extern uint8_t mouse_buttons;

#endif
