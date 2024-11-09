/*
 * rtc.c
 *
 * Created: 09/11/2024 07:11:10
 *  Author: Martynas
 */ 


#include "rtc.h"

// Global variable to hold the overflow callback
void (*RTC_OVF_isr_cb)(void) = NULL;

int8_t RTC_Initialize(void) {
	while (RTC.STATUS > 0) {
		// Wait for all RTC registers to be synchronized
	}
	
	// Set the desired period for wake-up (0xFF = longer interval)
	RTC.PER = 0xFF;
	
	// Select external 32.768 kHz clock
	RTC.CLKSEL = RTC_CLKSEL_XOSC32K_gc;
	
	// Enable overflow interrupt
	RTC.INTCTRL = RTC_OVF_bm;
	
	// Enable RTC with maximum prescaler and disable RUNSTDBY
	RTC.CTRLA = RTC_PRESCALER_DIV32768_gc | RTC_RTCEN_bm;
	
	return 0;
}

void RTC_Start(void) {
	RTC.CTRLA |= RTC_RTCEN_bm;
}

void RTC_SetOVFIsrCallback(RTC_cb_t cb) {
	RTC_OVF_isr_cb = cb;
}

void RTC_EnableOVFInterrupt(void) {
	RTC.INTCTRL |= RTC_OVF_bm;
}

void RTC_ClearOVFInterruptFlag(void) {
	RTC.INTFLAGS = RTC_OVF_bm;
}

ISR(RTC_CNT_vect) {
	if (RTC.INTFLAGS & RTC_OVF_bm) {
		if (RTC_OVF_isr_cb != NULL) {
			(*RTC_OVF_isr_cb)();  // Call the registered callback
		}
		RTC_ClearOVFInterruptFlag();
	}
}
