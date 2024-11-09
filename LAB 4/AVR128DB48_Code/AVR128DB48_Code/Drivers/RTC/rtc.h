/*
 * rtc.h
 *
 * Created: 09/11/2024 07:11:20
 *  Author: Martynas
 */ 

#ifndef RTC_H
#define RTC_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

// Callback types for RTC
typedef void (*RTC_cb_t)(void);

// RTC Functions
int8_t RTC_Initialize(void);
void RTC_Start(void);
void RTC_SetOVFIsrCallback(RTC_cb_t cb);
void RTC_EnableOVFInterrupt(void);
void RTC_ClearOVFInterruptFlag(void);

#endif // RTC_H
