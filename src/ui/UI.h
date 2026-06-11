#ifndef UI_H
#define UI_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void UI_Init(void);
void UI_Task(uint32_t now_ms);
uint8_t UI_LastKey(char *key);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
