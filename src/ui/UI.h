#ifndef UI_H
#define UI_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_task(uint32_t now_ms);
uint8_t ui_last_key(char *key);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
