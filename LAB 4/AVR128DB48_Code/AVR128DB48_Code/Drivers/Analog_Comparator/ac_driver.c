/*
 * ac_driver.c
 *
 * Created: 09/11/2024 00:06:37
 *  Author: Martynas
 */ 



#include "ac_driver.h"



void ac_driver_init(void) {
	// Set pin PD2 (port D, pin 2) as an input
	PORTD.DIRCLR = PIN2_bm;
	
	// Disable digital input buffer and pull-up resistor for pin PD2
	PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
	
	// Select the positive (AINP0 for pin PD2) and negative input (DACREF) source by writing to the MUXPOS and MUXNEG bit fields
	// Hint: AC_MUXPOS_AINP0_gc | AC_MUXNEG_DACREF_gc;
	AC0.MUXCTRL = AC_MUXPOS_AINP0_gc | AC_MUXNEG_DACREF_gc;
	
	// Enable the AC
	uint8_t AC_CTRLA_ENABLE = 0x01; // Bit 0 - ENABLE?DAC Enable
	AC0.CTRLA = AC_CTRLA_ENABLE;
	
	// Set Voltage Reference
	// V_REF = 1.024 V (MUST BE as it is stated in the task)
	VREF.ACREF = VREF_REFSEL_1V024_gc;
	
	// Set DACREF
	// V_DACREF = 0.1 V (MUST BE as it is stated in the task)
	// V_REF = 1.024 V (MUST BE as it is stated in the task)
	// Calculated DACREF value: DACREF = (V_DACREF*256)/V_REF = (0.1*256)/1.024 = 25
	AC0.DACREF = 25; // Bits 7:0 – DACREF[7:0]?DACREF Data Value (0 - 255)
}



uint8_t ac_driver_get_status() {
	// Bit shift output to only return: Bit 4 – CMPSTATE?AC State
	// MAsk the output to ensure it outputs only the bit we want and ignores the rest
	return ((AC0.STATUS >> AC_CMPSTATE_bp) & 0x01);
}



