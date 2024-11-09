/*
 * interrupt.c
 *
 * Created: 09/11/2024 02:25:18
 *  Author: Martynas
 */ 



#include "interrupt.h"



void interrupt_init() {
	// Initialize Peripherals ----------
	io_driver_disable(); // Makes sure to disable all pins at startup so we don't draw to much power
	ac_driver_init();
	led_driver_init();

	// AC Interrupt Initialization ----------
	// Disable global interrupts during setup
	cli();
	
	// In order to connect AC to interrupts we need to set up some extra things
	// We need to change mode of operation in AC register CTRLA
	// Then we need to enable event interrupt in AC register INTCTRL
	AC0.CTRLA = AC_ENABLE_bm | AC_RUNSTDBY_bm; // Enable AC, run in standby mode
	AC0.INTCTRL = AC_CMP_bm; // Enable interrupt on output change
	
	// Enable global interrupts after setup
	sei();
}



// ISR for the Analog Comparator interrupt
ISR(AC0_AC_vect) {
	// Get light level status
	uint8_t light_status = ac_driver_get_status();
	
	// Logic for LED
	if (light_status == 0) {
		led_driver_set_led_on();
	}
	else {
		led_driver_set_led_off();
	}
	
	// Clear the interrupt flag by writing 1 to it
	AC0.STATUS = AC_CMPIF_bm;
}