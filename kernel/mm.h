#ifndef MM_H
#define MM_H

typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;

void     mm_init(uint32_t heap_start, uint32_t heap_end);
void    *kmalloc(uint32_t size);
void     kfree(void *ptr);
uint32_t mm_used(void);
uint32_t mm_total(void);

#endif
