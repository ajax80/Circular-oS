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

#ifndef AUDIO_H
#define AUDIO_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

void audio_init(void);
void audio_tick(void);   /* call from main loop — drives note queue */

/* direct play — blocks via timer_wait_ms */
void audio_tone(uint32_t freq, uint32_t ms);
void audio_silence(uint32_t ms);

/* schema-driven groove */
void audio_play_schema(uint8_t state);   /* play signature for a state */
void audio_start_groove(void);           /* start background groove loop */
void audio_stop_groove(void);
void audio_set_corruption(int level);    /* 0=clean 1=drift 2=wrong 3=drop */

/* speaker primitives */
void spk_on(uint32_t freq);
void spk_off(void);

#endif
