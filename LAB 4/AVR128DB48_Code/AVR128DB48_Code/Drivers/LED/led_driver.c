/*
 * led_driver.c
 *
 * Created: 09/11/2024 00:43:43
 *  Author: Martynas
 */ 



#include "led_driver.h"



void led_driver_init(void) {
	PORTA.DIRSET = PIN2_bm;
}



void led_driver_set_led_on(void) {
	// LED is active low. Set pin LOW to turn LED on
	PORTA.OUTCLR = PIN2_bm;
}

void led_driver_set_led_off(void) {
	// LED is active low. Set pin HIGH to turn LED off
	PORTA.OUTSET = PIN2_bm;
}