#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Servo_Init(void);
void Servo_SetAngle(uint8_t degrees);
void Servo_Open(void);
void Servo_Close(void);

#ifdef __cplusplus
}
#endif

#endif
