/*
 * main.c
 *
 * Created: 11/8/2024 11:08:40 PM
 *  Author: Martynas
 */ 



// Define CPU Speed
#define F_CPU 4000000UL // 4.0 MHz



// AVR Libraries
#include <xc.h>
//#include <util/delay.h>
//#include <avr/sleep.h>



// Custom Libraries
#include "Drivers/IO/io_driver.h"
#include "Drivers/USART/usart.h"
#include "Drivers/Analog_Comparator/ac_driver.h"
#include "Drivers/LED/led_driver.h"
//#include "Drivers/Polling/polling.h"
//#include "Drivers/Interrupt/interrupt.h"
//#include "Drivers/Core/core.h"
#include "Drivers/RTC/rtc.h"



// Callback function to handle LED toggle based on AC status
void rtc_overflow_callback(void) {
	// Get light level status from AC
	uint8_t light_status = ac_driver_get_status();

	// Control LED based on AC status
	if (light_status == 0) {
		led_driver_set_led_on();
		} else {
		led_driver_set_led_off();
	}
}



int main(void)
{
	// Initialization ----------
	//io_driver_disable(); // Makes sure to disable all pins at startup so we don't draw to much power
	//USART3_Init(); // Debugging
	//ac_driver_init();
	//led_driver_init();
	
	//USART3_SendString("Initialization Complete :)"); // Debugging
	
	// Task 3: Pooling Initialization
	//polling_init();
	//set_sleep_mode(SLEEP_MODE_STANDBY); // Initialize standby sleep mode
	
	// Task 4: Interrupt Initialization
	//interrupt_init();
	//set_sleep_mode(SLEEP_MODE_STANDBY); // Initialize standby sleep mode
	
	// Task 5: Core Independent Operations Initialization
	//core_init();
	
	// Task 6: Low Power
	// Set up peripherals
	io_driver_disable();        // Disable unnecessary I/O for low power
	ac_driver_init();           // Initialize the Analog Comparator (AC)
	led_driver_init();          // Initialize LED 
	// Initialize and start RTC with overflow callback
	RTC_Initialize();
	RTC_SetOVFIsrCallback(rtc_overflow_callback);
	RTC_EnableOVFInterrupt();
	RTC_Start();
	// Enable global interrupts
	sei();
	// Enter power-down sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	
	
	// Infinite Loop
    while(1)
    {
		// Debugging Test (START) --------------------------------------------------
		/*
		char test[30] = "Status:     0\n\r"; // Reserve space for <NUM>, newline, and carriage return
		test[29] = '\0'; // Null terminator for the end of the message
		
		for (uint8_t i = 0; i < 10; i++) {
			// Get light level status
			uint8_t light_status = ac_driver_get_status();
			
			// Print out light level status
			test[12] = light_status + 0x30; // In ASCII numbers start from hex 0x30
			USART3_SendString(test);
			
			// Logic for LED
			if (light_status == 0) {
				led_driver_set_led_on();
			}
			else {
				led_driver_set_led_off();
			}
			
			// Small delay
			_delay_ms(1000);
		}
		*/
		// Debugging Test (STOP) --------------------------------------------------
		
		
		
		// Task 3 busy-waiting (START) --------------------------------------------------
		/*
		// GOAL   < 1.3 mA
		// RESULT = 1.1195 mA
		// Get light level status
		uint8_t light_status = ac_driver_get_status();
		
		// Logic for LED
		if (light_status == 0) {
			led_driver_set_led_on();
		}
		else {
			led_driver_set_led_off();
		}
		*/
		// Task 3 busy-waiting (STOP) --------------------------------------------------
		
		
		
		// Task 3 polling (START) --------------------------------------------------
		/*
		// GOAL   < 700 uA
		// RESULT = 679.3 uA
		// Go into sleep mode
		// LED Is controlled through Polling Code
		sleep_mode();
		*/
		// Task 3 polling (STOP) --------------------------------------------------
		
		
		
		// Task 4 interrupts (START) --------------------------------------------------
		
		// GOAL   < 200 uA
		// RESULT = 125.0 uA
		// Go into sleep mode
		// LED Is controlled through Interrupt Code
		//sleep_mode();
		// Task 4 interrupts (STOP) --------------------------------------------------
		
		
		
		// Task 5 Core (START) --------------------------------------------------
		// GOAL   < 150 uA
		// RESULT = 162.7 uA
		// Everything is ran in Core Code
		// CPU is off
		// The Events are happening outside CPU
		// Task 5 Core (STOP) --------------------------------------------------
		
		
		
		// Task 6 Low Power (START) --------------------------------------------------
		// GOAL   < AS SALL AS POSSIBLE!!!
		// RESULT = 162.7 uA
		// Everything is ran in Core Code
		// CPU is off
		// The Events are happening outside CPU
		
		// Go to sleep and wait for RTC overflow to wake up
		sleep_enable();
		sleep_cpu();  // CPU will sleep until woken by RTC interrupt
		sleep_disable();  // Disable sleep after waking up
		// Task 6 Low Power (STOP) --------------------------------------------------
    }
}