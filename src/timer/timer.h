#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     Timer_Init(void);
void     Timer_Task(void);   /* compatibility no-op; tick comes from ISR */
uint32_t Timer_Millis(void);
uint32_t Timer_GetMs(void);  /* compatibility wrapper */
uint8_t  Timer_Expired(uint32_t start_ms, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif
