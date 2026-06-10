#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void keypad_init(void);
void keypad_scan(uint32_t now_ms);
char keypad_get_key(void);

#ifdef __cplusplus
}
#endif

#endif
