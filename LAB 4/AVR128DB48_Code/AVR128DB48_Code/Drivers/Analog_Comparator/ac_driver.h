/*
 * ac_driver.h
 *
 * Created: 09/11/2024 00:06:23
 *  Author: Martynas
 */ 


#ifndef AC_DRIVER_H_
#define AC_DRIVER_H_



// Libraries for making ADC work properly
#include <xc.h>



// Functions
void ac_driver_init(void);

uint8_t ac_driver_get_status();



#endif /* AC_DRIVER_H_ */