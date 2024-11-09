/*
 * led_driver.h
 *
 * Created: 09/11/2024 00:43:55
 *  Author: Martynas
 */ 


#ifndef LED_DRIVER_H_
#define LED_DRIVER_H_



// Libraries for making LED work properly
#include <xc.h>
#include <avr/io.h>



// Functions
void led_driver_init(void);

void led_driver_set_led_on(void);
void led_driver_set_led_off(void);



#endif /* LED_DRIVER_H_ */