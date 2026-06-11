#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Keypad_Init(void);
void Keypad_Scan(uint32_t now_ms);
char Keypad_GetKey(void);

#ifdef __cplusplus
}
#endif

#endif
