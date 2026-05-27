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

#include "fb.h"

#define HEADER_ROWS      2
#define TASKBAR_HEIGHT  16
#define HDR_BG      0x1A1A2E
#define HDR_TITLE   0x00FF41
#define HDR_DIM     0x555577
#define HDR_STATE   0xFFAA00
#define TERM_BG     0x0D0D0D
#define TERM_FG     0x00CC33

/* 8x8 bitmap font, ASCII 0x20-0x7F */
static const unsigned char font8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 20 sp */
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, /* 21 !  */
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, /* 22 "  */
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, /* 23 #  */
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, /* 24 $  */
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, /* 25 %  */
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, /* 26 &  */
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, /* 27 '  */
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, /* 28 (  */
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, /* 29 )  */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, /* 2A *  */
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, /* 2B +  */
    {0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00}, /* 2C ,  */
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, /* 2D -  */
    {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00}, /* 2E .  */
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, /* 2F /  */
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, /* 30 0  */
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, /* 31 1  */
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, /* 32 2  */
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, /* 33 3  */
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, /* 34 4  */
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, /* 35 5  */
    {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, /* 36 6  */
    {0x7E,0x06,0x06,0x0C,0x18,0x30,0x30,0x00}, /* 37 7  */
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, /* 38 8  */
    {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}, /* 39 9  */
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, /* 3A :  */
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, /* 3B ;  */
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, /* 3C <  */
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, /* 3D =  */
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00}, /* 3E >  */
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00}, /* 3F ?  */
    {0x3E,0x63,0x6F,0x69,0x6F,0x60,0x3E,0x00}, /* 40 @  */
    {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00}, /* 41 A  */
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, /* 42 B  */
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, /* 43 C  */
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, /* 44 D  */
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, /* 45 E  */
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, /* 46 F  */
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, /* 47 G  */
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, /* 48 H  */
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* 49 I  */
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, /* 4A J  */
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, /* 4B K  */
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, /* 4C L  */
    {0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00}, /* 4D M  */
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, /* 4E N  */
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, /* 4F O  */
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, /* 50 P  */
    {0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00}, /* 51 Q  */
    {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, /* 52 R  */
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, /* 53 S  */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, /* 54 T  */
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, /* 55 U  */
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, /* 56 V  */
    {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00}, /* 57 W  */
    {0xC6,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0x00}, /* 58 X  */
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, /* 59 Y  */
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, /* 5A Z  */
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, /* 5B [  */
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, /* 5C \  */
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, /* 5D ]  */
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, /* 5E ^  */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, /* 5F _  */
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, /* 60 `  */
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, /* 61 a  */
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, /* 62 b  */
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, /* 63 c  */
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, /* 64 d  */
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, /* 65 e  */
    {0x1C,0x30,0x30,0x7C,0x30,0x30,0x30,0x00}, /* 66 f  */
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x7C}, /* 67 g  */
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, /* 68 h  */
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, /* 69 i  */
    {0x06,0x00,0x06,0x06,0x06,0x06,0x66,0x3C}, /* 6A j  */
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, /* 6B k  */
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* 6C l  */
    {0x00,0x00,0xCC,0xFE,0xFE,0xD6,0xC6,0x00}, /* 6D m  */
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, /* 6E n  */
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, /* 6F o  */
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, /* 70 p  */
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, /* 71 q  */
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, /* 72 r  */
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, /* 73 s  */
    {0x18,0x18,0x7E,0x18,0x18,0x18,0x0E,0x00}, /* 74 t  */
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, /* 75 u  */
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, /* 76 v  */
    {0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00}, /* 77 w  */
    {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00}, /* 78 x  */
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x7C}, /* 79 y  */
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, /* 7A z  */
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, /* 7B {  */
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, /* 7C |  */
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, /* 7D }  */
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}, /* 7E ~  */
    {0xFF,0x81,0xBD,0xA5,0xBD,0x81,0xFF,0x00}, /* 7F del*/
};

static unsigned int fb_addr   = 0;
static unsigned int fb_pitch  = 0;
static unsigned int fb_bpp    = 0;
static int fb_col = 0, fb_row = 0;
static int fb_cols = 0, fb_rows = 0;
static int fb_w_px = 0, fb_h_px = 0;
static int fb_split_x = 0;

static unsigned int fb_fg = TERM_FG;
static unsigned int fb_bg = TERM_BG;

/* cursor save-behind */
#define CUR_W 8
#define CUR_H 8
static unsigned int cur_saved[CUR_W * CUR_H];
static int cur_sx = -1, cur_sy = -1;

void fb_set_fg(unsigned int c) { fb_fg = c; }
void fb_set_bg(unsigned int c) { fb_bg = c; }

static void fb_put_pixel(unsigned int x, unsigned int y, unsigned int color) {
    unsigned char *p = (unsigned char *)(fb_addr + y * fb_pitch + x * (fb_bpp >> 3));
    p[0] =  color        & 0xFF;
    p[1] = (color >>  8) & 0xFF;
    p[2] = (color >> 16) & 0xFF;
    if (fb_bpp == 32) p[3] = 0;
}

static void fb_fill_rect(unsigned int x, unsigned int y,
                          unsigned int w, unsigned int h, unsigned int color) {
    unsigned int px, py;
    for (py = y; py < y + h; py++)
        for (px = x; px < x + w; px++)
            fb_put_pixel(px, py, color);
}

static void fb_draw_char_ex(int c, int col, int row,
                             unsigned int fg, unsigned int bg) {
    const unsigned char *glyph;
    int px, py;
    if (c < 0x20 || c > 0x7F) c = '?';
    glyph = font8x8[(unsigned char)c - 0x20];
    for (py = 0; py < 8; py++) {
        unsigned char bits = glyph[py];
        for (px = 0; px < 8; px++) {
            unsigned int clr = (bits & (0x80 >> px)) ? fg : bg;
            fb_put_pixel((unsigned int)(col * 8 + px),
                         (unsigned int)(row * 8 + py), clr);
        }
    }
}

static void fb_str_at(const char *s, int col, int row,
                       unsigned int fg, unsigned int bg) {
    while (*s) fb_draw_char_ex(*s++, col++, row, fg, bg);
}

static void fb_draw_char(int c, int col, int row) {
    fb_draw_char_ex(c, col, row, fb_fg, fb_bg);
}

void fb_draw_header(const char *state) {
    int hcols = fb_w_px / 8;
    if (!fb_addr) return;
    fb_fill_rect(0, 0, (unsigned int)fb_w_px, 16, HDR_BG);
    fb_str_at("CIRCULAR OS", 1, 0, HDR_TITLE, HDR_BG);
    if (hcols > 20) {
        fb_str_at("STATE:", hcols - 19, 0, HDR_DIM,   HDR_BG);
        fb_str_at(state,    hcols - 12, 0, HDR_STATE, HDR_BG);
    }
    fb_fill_rect(0, 15, (unsigned int)fb_w_px, 1, HDR_TITLE);
}

void fb_update_state(const char *state) {
    int hcols = fb_w_px / 8;
    if (!fb_addr || hcols <= 20) return;
    fb_fill_rect((unsigned int)((hcols - 12) * 8), 0,
                 (unsigned int)(12 * 8), 8, HDR_BG);
    fb_str_at(state, hcols - 12, 0, HDR_STATE, HDR_BG);
}

int fb_get_w(void) { return fb_w_px; }
int fb_get_h(void) { return fb_h_px; }

static void fb_str_px(const char *s, int x, int y,
                      unsigned int fg, unsigned int bg) {
    while (*s) {
        const unsigned char *glyph;
        int px, py, c = (unsigned char)*s++;
        if (c < 0x20 || c > 0x7F) c = '?';
        glyph = font8x8[c - 0x20];
        for (py = 0; py < 8; py++) {
            for (px = 0; px < 8; px++) {
                unsigned int clr = (glyph[py] & (0x80 >> px)) ? fg : bg;
                fb_put_pixel((unsigned int)(x + px), (unsigned int)(y + py), clr);
            }
        }
        x += 8;
    }
}

void fb_init(unsigned int addr, unsigned int pitch,
             unsigned int w, unsigned int h, unsigned int bpp) {
    fb_addr    = addr;
    fb_pitch   = pitch;
    fb_bpp     = bpp;
    fb_w_px    = (int)w;
    fb_h_px    = (int)h;
    fb_split_x = fb_w_px / 2;
    fb_cols    = fb_split_x / 8;
    fb_rows    = (int)(h / 8) - (TASKBAR_HEIGHT / 8);
    fb_col     = 0;
    fb_row     = HEADER_ROWS;
    fb_fill_rect(0, 0, w, h, TERM_BG);
}

int fb_active(void) {
    return fb_addr != 0;
}

static void fb_scroll(void) {
    int r, x;
    int bpp_bytes  = (int)(fb_bpp >> 3);
    int left_w     = fb_split_x ? fb_split_x : (fb_cols * 8);
    int left_bytes = left_w * bpp_bytes;
    unsigned char *base = (unsigned char *)fb_addr;
    int start_y = HEADER_ROWS * 8;
    int end_y   = (fb_rows - 1) * 8;

    for (r = start_y; r < end_y; r++) {
        unsigned char *dst = base + (unsigned int)r * fb_pitch;
        unsigned char *src = dst + (unsigned int)8 * fb_pitch;
        for (x = 0; x < left_bytes; x++)
            dst[x] = src[x];
    }
    fb_fill_rect(0, (unsigned int)(fb_rows - 1) * 8,
                 (unsigned int)left_w, 8, fb_bg);
    fb_row = fb_rows - 1;
}

void fb_draw_taskbar(const char *state, int online, int total, int mx, int my) {
    if (!fb_addr) return;
    int ty = fb_h_px - TASKBAR_HEIGHT;
    fb_fill_rect(0, (unsigned int)ty, (unsigned int)fb_w_px, 1, HDR_TITLE);
    fb_fill_rect(0, (unsigned int)(ty + 1), (unsigned int)fb_w_px,
                 TASKBAR_HEIGHT - 1, HDR_BG);

    /* left: OS name */
    fb_str_px("CIRCULAR OS", 4, ty + 4, HDR_TITLE, HDR_BG);

    /* centre: state + node count */
    {
        char buf[24];
        char *p = buf;
        const char *s = state;
        while (*s) *p++ = *s++;
        *p++ = ' ';
        *p++ = online / 10 + '0';
        *p++ = online % 10 + '0';
        *p++ = '/';
        *p++ = total / 10 + '0';
        *p++ = total % 10 + '0';
        *p++ = ' ';
        *p++ = 'n';
        *p++ = 'd';
        *p   = 0;
        int cx = (fb_w_px - (int)(p - buf) * 8) / 2;
        fb_str_px(buf, cx, ty + 4, HDR_STATE, HDR_BG);
    }

    /* right: cursor position */
    {
        char buf[16];
        char *p = buf;
        *p++ = 'x';
        *p++ = ':';
        if (mx >= 100) *p++ = '0' + mx / 100;
        *p++ = '0' + (mx % 100) / 10;
        *p++ = '0' + mx % 10;
        *p++ = ' ';
        *p++ = 'y';
        *p++ = ':';
        if (my >= 100) *p++ = '0' + my / 100;
        *p++ = '0' + (my % 100) / 10;
        *p++ = '0' + my % 10;
        *p   = 0;
        int rx = fb_w_px - (int)(p - buf) * 8 - 4;
        fb_str_px(buf, rx, ty + 4, 0x888888, HDR_BG);
    }
}

void fb_erase_cursor(void) {
    if (cur_sx < 0) return;
    int x, y;
    for (y = 0; y < CUR_H; y++)
        for (x = 0; x < CUR_W; x++) {
            unsigned int px = (unsigned int)(cur_sx + x);
            unsigned int py = (unsigned int)(cur_sy + y);
            if (px < (unsigned int)fb_w_px && py < (unsigned int)fb_h_px)
                fb_put_pixel(px, py, cur_saved[y * CUR_W + x]);
        }
    cur_sx = cur_sy = -1;
}

void fb_draw_cursor(int x, int y) {
    static const unsigned char shape[CUR_H] = {
        0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xE0, 0xA0, 0x10
    };
    int px, py;
    /* save behind */
    cur_sx = x; cur_sy = y;
    for (py = 0; py < CUR_H; py++)
        for (px = 0; px < CUR_W; px++) {
            unsigned int ppx = (unsigned int)(x + px);
            unsigned int ppy = (unsigned int)(y + py);
            if (ppx < (unsigned int)fb_w_px && ppy < (unsigned int)fb_h_px) {
                unsigned char *ptr = (unsigned char *)(fb_addr + ppy * fb_pitch + ppx * (fb_bpp >> 3));
                cur_saved[py * CUR_W + px] = (unsigned int)ptr[0]
                    | ((unsigned int)ptr[1] << 8)
                    | ((unsigned int)ptr[2] << 16);
            }
        }
    /* draw */
    for (py = 0; py < CUR_H; py++)
        for (px = 0; px < CUR_W; px++) {
            unsigned int ppx = (unsigned int)(x + px);
            unsigned int ppy = (unsigned int)(y + py);
            if (ppx < (unsigned int)fb_w_px && ppy < (unsigned int)fb_h_px) {
                if (shape[py] & (0x80 >> px))
                    fb_put_pixel(ppx, ppy, 0xFFFFFF);
            }
        }
}

void fb_draw_node_grid(unsigned int *colors, char *glyphs, int count, int selected) {
    int panel_w, panel_h, cell, step, off_x, off_y;
    int i, c, r, nx, ny;
    unsigned int color, dim;
    char buf[2];

    if (!fb_addr || fb_split_x <= 0) return;

    panel_w = fb_w_px - fb_split_x - 1;
    panel_h = fb_h_px - 32;

    fb_fill_rect((unsigned int)fb_split_x, 16, 1, (unsigned int)panel_h, 0x333333);
    fb_fill_rect((unsigned int)(fb_split_x + 1), 16,
                 (unsigned int)panel_w, (unsigned int)panel_h, TERM_BG);

    cell = panel_w < panel_h ? panel_w : panel_h;
    cell = (cell - 14) / 7;
    if (cell < 8) cell = 8;
    step = cell + 2;

    off_x = fb_split_x + 1 + (panel_w - 7 * step + 2) / 2;
    off_y = 16 + (panel_h - 7 * step + 2) / 2;

    buf[1] = 0;

    for (i = 0; i < 49 && i < count; i++) {
        color = colors[i];
        c = i % 7;
        r = i / 7;
        nx = off_x + c * step;
        ny = off_y + r * step;

        if (nx < 0 || ny < 0 || nx + cell > fb_w_px || ny + cell > fb_h_px)
            continue;

        if (color) {
            dim = (color >> 1) & 0x7F7F7F;
            fb_fill_rect((unsigned int)nx, (unsigned int)ny,
                         (unsigned int)cell, (unsigned int)cell, dim);
            /* border — top, bottom, left, right */
            fb_fill_rect((unsigned int)nx, (unsigned int)ny,
                         (unsigned int)cell, 1, color);
            fb_fill_rect((unsigned int)nx, (unsigned int)(ny + cell - 1),
                         (unsigned int)cell, 1, color);
            fb_fill_rect((unsigned int)nx, (unsigned int)ny,
                         1, (unsigned int)cell, color);
            fb_fill_rect((unsigned int)(nx + cell - 1), (unsigned int)ny,
                         1, (unsigned int)cell, color);
            if (cell >= 10) {
                buf[0] = glyphs[i];
                fb_str_px(buf, nx + (cell - 8) / 2, ny + (cell - 8) / 2, color, dim);
            }
            /* selected: white inner highlight */
            if (i == selected && cell >= 4) {
                fb_fill_rect((unsigned int)(nx+1), (unsigned int)(ny+1),
                             (unsigned int)(cell-2), 1, 0xFFFFFF);
                fb_fill_rect((unsigned int)(nx+1), (unsigned int)(ny+cell-2),
                             (unsigned int)(cell-2), 1, 0xFFFFFF);
                fb_fill_rect((unsigned int)(nx+1), (unsigned int)(ny+1),
                             1, (unsigned int)(cell-2), 0xFFFFFF);
                fb_fill_rect((unsigned int)(nx+cell-2), (unsigned int)(ny+1),
                             1, (unsigned int)(cell-2), 0xFFFFFF);
            }
        } else {
            fb_fill_rect((unsigned int)nx, (unsigned int)ny,
                         (unsigned int)cell, (unsigned int)cell, 0x111111);
            fb_fill_rect((unsigned int)nx, (unsigned int)ny,
                         (unsigned int)cell, 1, 0x2A2A2A);
            fb_fill_rect((unsigned int)nx, (unsigned int)(ny + cell - 1),
                         (unsigned int)cell, 1, 0x2A2A2A);
            fb_fill_rect((unsigned int)nx, (unsigned int)ny,
                         1, (unsigned int)cell, 0x2A2A2A);
            fb_fill_rect((unsigned int)(nx + cell - 1), (unsigned int)ny,
                         1, (unsigned int)cell, 0x2A2A2A);
        }
    }
}

int fb_node_hit_test(int mx, int my) {
    int panel_w, panel_h, cell, step, off_x, off_y;
    int c, r, nx, ny;

    if (!fb_addr || fb_split_x <= 0) return -1;
    if (mx <= fb_split_x || mx >= fb_w_px) return -1;
    if (my < 16 || my >= fb_h_px - 16) return -1;

    panel_w = fb_w_px - fb_split_x - 1;
    panel_h = fb_h_px - 32;
    cell = panel_w < panel_h ? panel_w : panel_h;
    cell = (cell - 14) / 7;
    if (cell < 8) cell = 8;
    step = cell + 2;
    off_x = fb_split_x + 1 + (panel_w - 7 * step + 2) / 2;
    off_y = 16 + (panel_h - 7 * step + 2) / 2;

    for (r = 0; r < 7; r++) {
        for (c = 0; c < 7; c++) {
            nx = off_x + c * step;
            ny = off_y + r * step;
            if (mx >= nx && mx < nx + cell && my >= ny && my < ny + cell)
                return r * 7 + c;
        }
    }
    return -1;
}

void fb_draw_node_detail(const char *line1, const char *line2, unsigned int color) {
    int detail_y, panel_w;

    if (!fb_addr || fb_split_x <= 0) return;

    panel_w  = fb_w_px - fb_split_x - 1;
    detail_y = fb_h_px - 16 - 48;

    fb_fill_rect((unsigned int)(fb_split_x + 1), (unsigned int)detail_y,
                 (unsigned int)panel_w, 48, 0x111111);
    fb_fill_rect((unsigned int)(fb_split_x + 1), (unsigned int)detail_y,
                 (unsigned int)panel_w, 1, 0x333333);

    if (line1 && *line1)
        fb_str_px(line1, fb_split_x + 4, detail_y + 6,  color,    0x111111);
    if (line2 && *line2)
        fb_str_px(line2, fb_split_x + 4, detail_y + 22, 0xAAAAAA, 0x111111);
    if (!line1 || !*line1)
        fb_str_px("click a node", fb_split_x + 4, detail_y + 6, 0x444444, 0x111111);
}

void fb_putc(char c) {
    if (!fb_addr) return;
    if (c == '\n') {
        fb_col = 0;
        if (++fb_row >= fb_rows) fb_scroll();
    } else if (c == '\r') {
        fb_col = 0;
    } else if (c == '\b') {
        if (fb_col > 0) { fb_col--; fb_draw_char(' ', fb_col, fb_row); }
    } else {
        if (fb_col >= fb_cols) {
            fb_col = 0;
            if (++fb_row >= fb_rows) fb_scroll();
        }
        fb_draw_char((unsigned char)c, fb_col++, fb_row);
    }
}
