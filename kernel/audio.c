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

#include "audio.h"
#include "timer.h"

/* ── I/O ─────────────────────────────────────────────────────── */
static void outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static unsigned char inb(unsigned short p) {
    unsigned char v;
    __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));
    return v;
}

/* ── PC Speaker via PIT channel 2 ────────────────────────────── */
void spk_on(uint32_t freq) {
    if (!freq) { spk_off(); return; }
    uint32_t div = 1193180u / freq;
    outb(0x43, 0xB6);                        /* channel 2, lobyte/hibyte, square wave */
    outb(0x42, (unsigned char)(div & 0xFF));
    outb(0x42, (unsigned char)(div >> 8));
    outb(0x61, inb(0x61) | 0x03);           /* gate + speaker enable */
}

void spk_off(void) {
    outb(0x61, inb(0x61) & ~0x03u);
}

/* ── Note queue ──────────────────────────────────────────────── */
typedef struct { uint32_t freq; uint32_t dur_ms; } note_t;

#define QSIZE 64
static note_t  q[QSIZE];
static int     q_head = 0, q_tail = 0;
static uint32_t note_start_tick = 0;
static int      playing = 0;

static int q_push(uint32_t freq, uint32_t dur) {
    int next = (q_tail + 1) % QSIZE;
    if (next == q_head) return 0;   /* full */
    q[q_tail].freq   = freq;
    q[q_tail].dur_ms = dur;
    q_tail = next;
    return 1;
}

static void q_clear(void) { q_head = q_tail = 0; spk_off(); playing = 0; }

/* ── The Groove ──────────────────────────────────────────────── */
/*  A looping 8-note motif — clean signal = all nodes healthy    */
#define GROOVE_LEN 8
static const uint32_t groove_freq[GROOVE_LEN] = {
    392, 440, 494, 523, 494, 440, 392, 330
};
static const uint32_t groove_dur[GROOVE_LEN] = {
    150, 150, 150, 300, 150, 150, 300, 300
};

static int     groove_active  = 0;
static int     groove_pos     = 0;
static int     corrupt_level  = 0;   /* 0=clean, 1=drift, 2=wrong, 3=drop */

/* corruption tables */
static const uint32_t wrong_freq[GROOVE_LEN] = {
    370, 415, 466, 494, 523, 415, 370, 311   /* tritone-ish substitutions */
};

static void groove_queue_next(void) {
    uint32_t f = groove_freq[groove_pos];
    uint32_t d = groove_dur[groove_pos];

    switch (corrupt_level) {
        case 1:  /* timing drift — 30% longer, slight sharp */
            f = f + (f >> 4);
            d = d + (d >> 2);
            break;
        case 2:  /* wrong note — tritone sub, half duration */
            f = wrong_freq[groove_pos];
            d = d >> 1;
            break;
        case 3:  /* dropout — silence for this note */
            f = 0;
            break;
        default: break;
    }

    q_push(f, d);
    groove_pos = (groove_pos + 1) % GROOVE_LEN;
}

/* ── Schema state signatures ─────────────────────────────────── */
/* Played once on `schema_audio` command to announce a state */
static const uint32_t sig_fund[]   = {523, 659, 784, 0};  /* C-E-G major — go */
static const uint32_t sig_new[]    = {330, 392, 0};        /* low rise — young */
static const uint32_t sig_trust[]  = {523, 784, 0};        /* perfect fifth    */
static const uint32_t sig_settle[] = {440, 0};             /* single A — idle  */
static const uint32_t sig_recov[]  = {415, 494, 0};        /* minor — attention */
static const uint32_t sig_fric[]   = {466, 440, 466, 0};   /* wobble — failing */
static const uint32_t sig_exc[]    = {220, 0, 0};          /* low thud — broke */

static const uint32_t *state_sig(uint8_t state) {
    switch (state) {
        case 1:  return sig_fund;
        case 10: return sig_trust;
        case 88: return sig_fund;
        case 7:  return sig_settle;
        case 8:  return sig_new;
        case 9:  return sig_recov;
        case 6:  return sig_fric;
        case 76: return sig_exc;
        default: return sig_settle;
    }
}

void audio_play_schema(uint8_t state) {
    const uint32_t *sig = state_sig(state);
    q_clear();
    while (*sig) {
        q_push(*sig, 200);
        q_push(0, 60);   /* brief gap between notes */
        sig++;
    }
}

/* ── Public API ──────────────────────────────────────────────── */
void audio_init(void) {
    spk_off();
    q_head = q_tail = 0;
    playing = 0;
    groove_active = 0;
    corrupt_level = 0;
    groove_pos = 0;
}

void audio_start_groove(void) {
    q_clear();
    groove_active = 1;
    groove_pos    = 0;
    groove_queue_next();
}

void audio_stop_groove(void) {
    groove_active = 0;
    q_clear();
    spk_off();
}

void audio_set_corruption(int level) {
    corrupt_level = level;
}

void audio_tone(uint32_t freq, uint32_t ms) {
    q_push(freq, ms);
    q_push(0, 50);
}

void audio_silence(uint32_t ms) {
    q_push(0, ms);
}

/* ── Tick handler — call from main loop as fast as possible ─── */
void audio_tick(void) {
    uint32_t ticks = timer_poll();

    if (q_head == q_tail && !playing) {
        if (groove_active) groove_queue_next();
        else return;
    }

    if (!playing) {
        if (q_head == q_tail) return;
        note_t *n = &q[q_head];
        if (n->freq) spk_on(n->freq);
        else         spk_off();
        note_start_tick = ticks;
        playing = 1;
    }

    if (ticks - note_start_tick >= q[q_head].dur_ms) {
        q_head = (q_head + 1) % QSIZE;
        playing = 0;
        spk_off();
        if (groove_active && q_head == q_tail)
            groove_queue_next();
    }
}
