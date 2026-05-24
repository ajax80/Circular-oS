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
