/*
 * Circular OS
 * Copyright (C) 2026 Jonathan Eugene Ayers <ayersjon80@gmail.com>
 *
 * This file is part of Circular OS.
 *
 * Circular OS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Circular OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with Circular OS. If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * For commercial licensing, see COMMERCIAL_LICENSE in this repository.
 */

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
void fb_draw_taskbar(const char *state, int online, int total, int mx, int my);
void fb_draw_cursor(int x, int y);
void fb_erase_cursor(void);
int  fb_get_w(void);
int  fb_get_h(void);
void fb_draw_node_grid(unsigned int *colors, char *glyphs, int count, int selected);
int  fb_node_hit_test(int mx, int my);
void fb_draw_node_detail(const char *line1, const char *line2, unsigned int color);

#endif
