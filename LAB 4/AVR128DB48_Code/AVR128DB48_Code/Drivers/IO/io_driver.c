/*
 * io_driver.c
 *
 * Created: 09/11/2024 00:55:28
 *  Author: Martynas
 */ 



#include "io_driver.h"



void io_driver_disable() {
	// Set up the PINCONFIG register to enable pull-ups and disable digital input
	PORTA.PINCONFIG = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;

	// Apply the configuration across all ports using the multi-pin configuration
	PORTA.PINCTRLUPD = 0xFF; // Apply configuration to all pins in PORTA
	PORTB.PINCTRLUPD = 0xFF; // Apply configuration to all pins in PORTB
	PORTC.PINCTRLUPD = 0xFF; // Apply configuration to all pins in PORTC
	PORTD.PINCTRLUPD = 0xFF; // Apply configuration to all pins in PORTD
	PORTE.PINCTRLUPD = 0xFF; // Apply configuration to all pins in PORTE
}