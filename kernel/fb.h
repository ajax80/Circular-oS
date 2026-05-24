#ifndef FB_H
#define FB_H

void fb_init(unsigned int addr, unsigned int pitch,
             unsigned int w, unsigned int h, unsigned int bpp);
void fb_putc(char c);
int  fb_active(void);
void fb_set_fg(unsigned int c);
void fb_set_bg(unsigned int c);
void fb_draw_header(const char *state);
void fb_update_state(const char *state);

#endif
