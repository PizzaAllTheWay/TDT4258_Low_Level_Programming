/*
 * polling.h
 *
 * Created: 09/11/2024 01:20:22
 *  Author: Martynas
 */ 


#ifndef POLLING_H_
#define POLLING_H_



// Libraries for making Polling work properly
#include <xc.h>
#include <avr/io.h>
#include <avr/interrupt.h>



// Since we are in polling mode we will run everything in timer interrupts mode
// Thats why we use all the code in the Timer interrupt service routine function
#include "../IO/io_driver.h"
#include "../Analog_Comparator/ac_driver.h"
#include "../LED/led_driver.h"



// Functions
void polling_init();



#endif /* POLLING_H_ */