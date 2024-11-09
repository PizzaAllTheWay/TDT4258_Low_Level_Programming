/*
 * polling.c
 *
 * Created: 09/11/2024 01:20:35
 *  Author: Martynas
 */ 



#include "polling.h"



// Set the period of the timer. PER = period[s] * F_CPU / Prescaler = 0.01s * 4 000 000 Hz / 1024
#define F_CPU 4000000UL
#define PRESCALE 1024
#define PER(PERIOD_S) ((PERIOD_S * F_CPU)/PRESCALE)



void polling_init() {
	// Initialize Peripherals ----------
	io_driver_disable(); // Makes sure to disable all pins at startup so we don't draw to much power
	ac_driver_init();
	led_driver_init();
	
	// Initialize Timer TC0 ----------
	// Disable global interrupts
	cli();
	
	// Set the period of the timer
	// Period = 0.1 s
	TCA0.SINGLE.PER = PER(0.1);
	
	// Enable timer overflow interrupt
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	
	// Enable timer
	// We enable standby mode so that we can have timer while we are in sleep mode
	// We want timer to run as slow as possible so to conserve as much energy as possible, so we prescale clock with 1024
	// Then we enable timer
	//TCA0.SINGLE.CTRLA = TCA_SINGLE_RUNSTDBY_bm | TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_RUNSTDBY_bm | TCA_SINGLE_CLKSEL_DIV2_gc | TCA_SINGLE_ENABLE_bm;
	
	// Enable global interrupts 
	sei();
}



ISR(TCA0_OVF_vect) {
	// Every time microcontroller goes into sleep mode, it disabled AC
	// Thats why every time we get interrupt we reinitialize our AC to enable it again
	ac_driver_init();
	
	// Get light level status
	uint8_t light_status = ac_driver_get_status();
	
	// Logic for LED
	if (light_status == 0) {
		led_driver_set_led_on();
	}
	else {
		led_driver_set_led_off();
	}
	
	// Clear interrupt flag
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}