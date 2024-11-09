/*
 * core.c
 *
 * Created: 09/11/2024 02:52:25
 *  Author: Martynas
 */ 



#include "core.h"



void core_init() {
	// Voltage Optimization ----------
	// This is a very csarry thing to do.... F IT YOLO
	// Brownout detection, who needs that XP
	// Disable BOD entirely
	//BOD.CTRLA = (BOD.CTRLA & ~BOD_SLEEP_gm) | BOD_SLEEP_DIS_gc;  // Disable BOD in sleep
	BOD.CTRLA = BOD_SAMPFREQ_bm | (BOD.CTRLA & ~BOD_SLEEP_gm) | BOD_SLEEP_SAMPLED_gc;  // BOD sampled mode in sleep
	BOD.CTRLB = BOD_LVL_BODLEVEL3_gc; // Set threshold higher to consume less power
	
	SLPCTRL.VREGCTRL = SLPCTRL_PMODE0_bm;  // Set regulator mode to AUTO

	// Oscillator Optimizations ----------
	// Choose to use external Clock as main clock
	// Save power on using extrenal Crystal oscilator instead
	// Set up the external 32.768 kHz crystal oscillator
	CLKCTRL.OSC32KCTRLA = CLKCTRL_RUNSTBY_bm; // Ensure External Clock will run no matter what mode microcontroller is in
	CLKCTRL.XOSC32KCTRLA = CLKCTRL_RUNSTBY_bm | CLKCTRL_CSUT_64K_gc | CLKCTRL_LPMODE_bm | CLKCTRL_ENABLE_bm; // Enable the 32.768 kHz crystal
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_RUNSTDBY_bm | CLKCTRL_FRQRANGE_8M_gc | CLKCTRL_ENABLE_bm; // Enable high frequncu clock to be external clock with prescaling to 8 MHz
	
	// Select the external oscillator as the main clock source
	// Also select CLKOUT to be from external clock
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_XOSC32K_gc | CLKCTRL_CLKOUT_bm | CLKCTRL_ENABLE_bm;
	
	// Set the clock prescaler to the maximum value (e.g., divide by 48)
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_48X_gc | CLKCTRL_PEN_bm; // Enable prescaler with division factor 256
	
	// Set external clock as PLL output instead of internal clock
	CLKCTRL.PLLCTRLA = CLKCTRL_SOURCE_bm;
	
	// Initialize Peripherals ----------
	// Makes sure to disable all pins at startup so we don't draw to much power
	io_driver_disable();
	
	// Initialize Core Independent Operations ----------
	// Step 1: Configure the Analog Comparator as Event Generator
	// Set the AC0 to generate events when crossing the threshold
	// Also set AC to PROFILE2 of power mode, lower power consumption from 70 to 12 uA
	// Also sleect hysteresis SMALL, so that one can have lower power consumption
	ac_driver_init();
	AC0.CTRLA = AC_ENABLE_bm | AC_RUNSTDBY_bm | AC_POWER_PROFILE2_gc | AC_HYSMODE0_bm; // Enable AC and allow it to run in standby
	
	// Step 2: Configure event user peripheral (ie our LED)
	led_driver_init();

	// Step 3: Configure Event System Channel for AC
	// Use Event Channel 0 and route the AC0 output to it
	EVSYS.CHANNEL0 = EVSYS_CHANNEL0_AC0_OUT_gc; // Route AC0 output to Event Channel 0

	// Step 4: Configure LED Pin as Event User
	// Route Event Channel 0 to the LED pin
	EVSYS.USEREVSYSEVOUTA = 0x01; // Use Channel 0 (0x01) for event output on PORTA (Where our LED is)

	// Step 5: Configure CPU into sleep mode
	set_sleep_mode(SLEEP_MODE_STANDBY); // Set standby mode for lowest power consumption	
	sleep_enable(); // Enable sleep mode
	sleep_cpu(); // Make CPU go to sleep
}