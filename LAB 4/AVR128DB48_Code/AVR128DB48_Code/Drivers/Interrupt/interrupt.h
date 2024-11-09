/*
 * interrupt.h
 *
 * Created: 09/11/2024 02:25:31
 *  Author: Martynas
 */ 


#ifndef INTERRUPT_H_
#define INTERRUPT_H_



// Libraries for making Interrupts work properly
#include <xc.h>
#include <avr/io.h>
#include <avr/interrupt.h>



// Since we are in interrupt mode we will run everything on interrupts
// Thats why we use all the code in the AC interrupt service routine function
#include "../IO/io_driver.h"
#include "../Analog_Comparator/ac_driver.h"
#include "../LED/led_driver.h"



// Functions
void interrupt_init();



#endif /* INTERRUPT_H_ */