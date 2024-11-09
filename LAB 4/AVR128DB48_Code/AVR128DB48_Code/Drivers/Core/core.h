/*
 * core.h
 *
 * Created: 09/11/2024 02:52:34
 *  Author: Martynas
 */ 


#ifndef CORE_H_
#define CORE_H_



// Libraries for making Core Independent Operations work properly
#include <xc.h>
#include <avr/io.h>
#include <avr/sleep.h>



// Since we are in core independent mode we will run everything outside CPU
// Thats why we need to make sure everything is set up before CPU goes to sleep
#include "../IO/io_driver.h"
#include "../Analog_Comparator/ac_driver.h"
#include "../LED/led_driver.h"



// Functions
void core_init();



#endif /* CORE_H_ */