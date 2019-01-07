/*

Date:	2016.11.22
Author:	Pete Mills

uses a single index mark, i.e. 1 ppr

*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~* Display States *~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
Pressing the input switch SW1 will cycle thru the available display units.

When SLEEP_ENABLE is defined, switchState = 0 puts tinyTach to sleep. The processor will wake up when 
SW1 is pressed again.

switchState		Bubble display / Serial output units
-----------		------------------------------------
0				Sleeps the processor until a button press wakes it up. - Only #ifdef SLEEP_ENABLE
1				Cycles per minute ( Cpm )
2				Cycles per second ( Hz )	
3				Period - Time between input pulses ( mS )		

*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~* Display States *~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*




*/

//~~~~~~~~~~~~~~~~~~~~~~******************** Includes ********************~~~~~~~~~~~~~~~~~~~~~~~~~

#include <avr/io.h>
#include <avr/interrupt.h>							
#include <util/delay.h>		
#include <avr/sleep.h>
#include <stdlib.h>
#include "main.h"
#include "drvr_gpio.h"
#include "drvr_bubble_display.h"
#include "drvr_serial.h"
#include "drvr_tach.h"
#include "drvr_watchdog.h"

#include "app_scheduler.h"
#include "task_serial.h"
#include "task_b.h"
#include "task_bubble_display.h"
#include "task_watchdog.h"

//~~~~~~~~~~~~~~~~~~~~~~******************** Includes ********************~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~************** Pre-Processor Directives *************~~~~~~~~~~~~~~~~~~~~~~~
#define SLEEP_ENABLE		// Hardware changes are necessary to make sleep modes useful.
#define SERIAL_ENABLE		// Also disables period output
#define PSTRETCH_ENABLE		// Pulse stretch output
//~~~~~~~~~~~~~~~~~~~~~************** Pre-Processor Directives *************~~~~~~~~~~~~~~~~~~~~~~~

/* The sysystem tick ISR fires at 524288 Hz, and it is an 8-bit timer
 * Therefore the tick counter rolls over at 524288 / 256 = 2048 Hz
 * Thus the tick time is 488 microseconds.
 * Actually, this back calculates to 2049.19 microseconds. Probably
 * ok here, but todo: look into this.
 */
#define TICK__TIME_US   488UL
#define TICK__2000MS    ( 2000000UL / TICK__TIME_US)
#define TICK__1000MS    ( 1000000UL / TICK__TIME_US)
#define TICK__100MS     ( 100000UL / TICK__TIME_US)
#define TICK__10MS      ( 10000UL / TICK__TIME_US)
#define TICK__1MS       ( 1000UL / TICK__TIME_US)
#define TICK__100US     ( 100UL / TICK__TIME_US)

#define CAPTURE_RESULT_READY	2


volatile uint16_t tmr0ZeroDisplay = 0;



#define LINE_FEED		10
#define CARRIAGE_RETURN	13






#define ARRAY_SIZE 			15		// moving average filter
#define CYCLE_FILTER_LIM	6554	// 6554 clock cycles ~= 600 RPM, Don't filter speed data if slower 

#define ZERO_DISPLAY_SPEED	3932	// 3932 clock cycles ~= 1000 RPM
									// Don't zero the display after a timeout if totClockCycles is less ( speed is faster )  
									// DO zero the display if you have never hit this threshold and a timeout has expired. 



// display update rates
#define TEN_SECONDS 	2550
#define ONE_SECOND 		255	// number of timer0 overflows in one second
#define HALF_SECOND		127
#define QUARTER_SECOND	63

#define SERIAL_SEND 	HALF_SECOND

#define PULSE_STRETCH		1		// one timer0 overflow is ~= 4mS duration for retransmitting the PPR
#define ZERO_DISPLAY_TIME	1280 	// timer0 overflows.

#define CLK2CPM 		3932160	// Numerator for converting clock cycles to cycles per minute. ( 65536 * 60 ) 
#define CLK2HZ			65536	// Numerator for converting clock cycles to cycles per second. ( 65536 * 1 )

#ifdef SLEEP_ENABLE
	#define MIN_SWITCHSTATE 0
#else
	#define MIN_SWITCHSTATE 1
#endif

#define MAX_SWITCHSTATE 3



//~~~~~~~~~~~~~~~~~~~~~******************** Definitions ********************~~~~~~~~~~~~~~~~~~~~~~~







//~~~~~~~~~~~~~~~~~~~******************** Global Variables ********************~~~~~~~~~~~~~~~~~~~~

volatile uint16_t bubbleOutput = 0;	// Sending data to the bubble display
uint32_t serialOutput = 0;			// Sending data to the UART
uint16_t cycPerMin = 0;				// DUT cycles per minute i.e. RPM
uint16_t cycPerSec = 0;				// DUT cycles per second i.e. Hz
uint32_t totClockCycles = 0;		// Clock cycles per part cycle i.e time per revolution

uint16_t filterArray[ ARRAY_SIZE ];

uint8_t switchState = 1;




#ifdef PSTRETCH_ENABLE
volatile uint8_t tmr0PulseStretch = 0;
#endif

#ifdef SERIAL_ENABLE
volatile uint8_t tmr0SerialPrint = 0;
#endif

volatile uint8_t DP0 = 0;
volatile uint8_t DP1 = 0;
volatile uint8_t DP2 = 0;
volatile uint8_t DP3 = 0;

uint8_t doZeroDisplay = 0;


static void sys_tick_isr_init(void)
{
	/* This timer overflow interrupt will  */
    TCCR0B |= ( 1 << CS01 );    /* Div8 on clock for 4.194304 MHz / 8 = 524288 Hz, */
                                /* so, 2048 8-bit timer overflows per second */
	
    TIMSK |= ( 1 << TOIE0 );	/* Enable timer 0 overflow interrupt. */
}

int main(void)
{
	Drvr_GPIO_Init();
	Drvr_Bubble_Display_Init();
	Drvr_Serial_Init();
	Drvr_Tach_Init();
    Drvr_Watchdog_Init();
    sys_tick_isr_init();

    /* Define the sleep mode type. */
    set_sleep_mode( SLEEP_MODE_PWR_DOWN );

    /* Global interrupt enable. */
    sei();



    /* Task list for the scheduler to run. */
    task_t tasks[] =
    {
  	    { TICK__1000MS ,  0, Task_Serial },
        { TICK__100MS ,  0, task_b },
        { TICK__10MS , 0, Task_Bubble_Display },
        { TICK__100MS , 0, Task_Data_Hold },
        { TICK__2000MS , 0, Task_Watchdog },
        /* End of the task list. */
  	    { 0 , 0, NULL }
    };

	char buff[10]; 

	Drvr_Serial_Print_String( "Reset.\n\r" );
    
	uint16_t data_hold = 0;

	while(1)
	{	

#if 1

	//_delay_ms(1000);
		uint16_t speed;
		speed++;

		if( speed == 7000 ){ speed = 0; }
	

        Task_Bubble_Display_Set_Bubble_Data( speed, 1 );
#endif
		/* Run all the tasks in tasks[] */
		App_Scheduler_Run_Tasks( tasks );
        Task_Bubble_Display_Set_Bubble_Data( 0, 0 );
	
		// Input switch state
		if( Drvr_GPIO_Switch_Is_Pressed() )
		{
			switchState++;
			
			if( switchState > MAX_SWITCHSTATE )
			{	
				switchState = MIN_SWITCHSTATE;
			}
			
			/* Show the new state on the bubble display for a while. */
			Task_Bubble_Display_Set_Bubble_Data( switchState, 5 );
			Task_Bubble_Display_Set_Data_Hold( 10 );
			
			Drvr_GPIO_Led_Off();
		}


		#if 0	
		// If a new frequency is ready for calculation
		if( capture_state >= CAPTURE_RESULT_READY )	
		{
			
			// Calculate the total number of system clock cycles between tach input pulses
			if ( input_cap_2 < input_cap_1 )
			{
				input_cap_2 += 65536;
			}
			
			totClockCycles = input_cap_2 - input_cap_1;
			totClockCycles += num_overflows_tmr_1 * 65536;
			
			
			// Prepare the data for outputting in the correct units.
			switch( switchState )
			{
				case 1:	// Cycles per minute ( CPM/RPM )
					/*
					// Extend the low range resolution. i.e where RPM < 1
					if( totClockCycles > CLK2CPM ){
						totClockCycles /= 1000; dont modify totClockcycles
						DP0 = 1;	// turn on the decimal point to show < 1 CPM.
					}else{
						DP0 = 0;
						DP1 = 0;
					}
					*/
					cycPerMin = CLK2CPM / totClockCycles; 	
					
					bubbleOutput = cycPerMin;
						
				break;
				
				case 2: // Cycles per Second ( Hz )
					
					// Extend the low range resolution. i.e where Hz < 1
					if( totClockCycles > CLK2HZ ){
						cycPerSec = CLK2HZ / totClockCycles / 1000;	// Hz
						DP0 = 1;	// turn on the decimal point to show < 1 CPM.
					}else{
						cycPerSec = CLK2HZ / totClockCycles;	// Hz
						DP0 = 0;
						DP1 = 0;
					}
					
					bubbleOutput = cycPerSec;
				
				break;
				
				case 3: // Period ( mS )
					bubbleOutput = ( totClockCycles * 1000 ) / 65536;
				break;
				
				
			}
			
			
			// If the reading is 10000 or above, shift to the right by one digit and drop the 
			// last digit. Also, light up the second decimal points so you know the reading is actually * 10
			if ( bubbleOutput > 9999 ){
				bubbleOutput /= 10;
				DP1 = 1;
			}else{
				DP1 = 0;	
			}
			
				
			
			// Use the moving average filter for the reading if the rate is high enough.
			if( totClockCycles > 0 && totClockCycles < CYCLE_FILTER_LIM )
			{
				bubbleOutput = filter( bubbleOutput );
			}else{
				filter( bubbleOutput );
			}
			
			// Set a separate serial output variable as it needs to be formatted differently.
			serialOutput = bubbleOutput;
			
			// we got that first cycle time, now we need to grab the subsequent ones
			input_cap_1 = input_cap_2;						// the timer is still running, so we prepare for the next calculation 
			capture_state = 1;							// Prepare the capture state machine
			num_overflows_tmr_1 = 0;
			TIMSK |= ( ( 1 << TOIE1 ) | ( 1 << ICIE1 ) );	// Enable timer capture and overflow interrupts
			
			
			// for the pulse stretch
			#ifdef PSTRETCH_ENABLE
				tmr0PulseStretch = 0; 
				Drvr_Tach_Rexmit_On();
			#endif
			
			// To visually check that we are getting a good tach pulse train.
			Drvr_GPIO_Led_Toggle();
			
			// Dont zero out the display; we have data!
			tmr0ZeroDisplay = 0;
		}
		

		
		
		// Turn off the retransmit bit if the ppr has been stretched enough
		#ifdef PSTRETCH_ENABLE
			if( tmr0PulseStretch >= PULSE_STRETCH )
			{
				Drvr_Tach_Rexmit_Off();	
			}
		#endif
		
		// Serial print
		#ifdef SERIAL_ENABLE
			if( tmr0SerialPrint >= SERIAL_SEND )
			{
				uartPrint( serialOutput, 0 );	// value
				serialWrite( 44 );				// ASCII Comma
				
				// print units
				switch(switchState)
				{
					case 1: // counts per min / RPM
						serialWrite( 67 );	// ASCII C
						serialWrite( 80 );	// ASCII P
						serialWrite( 77 );	// ASCII M
					break;
					
					case 2:	// Hz
						serialWrite( 72 );	// ASCII H
						serialWrite( 122 );	// ASCII z
					break;
					
					case 3:	// Period
						serialWrite( 109 );	// ASCII m
						serialWrite( 83 );	// ASCII S
					break;
				}
				
				serialWrite( 44 );				// ASCII Comma
				
				uartPrint( totClockCycles, 0 );	// Raw clock cycles
				
				serialWrite( 44 );	// ASCII Comma
				serialWrite( 67 );	// ASCII C
				serialWrite( 76 );	// ASCII L
				serialWrite( 75 );	// ASCII K
				
				serialWrite( LINE_FEED );
    			serialWrite( CARRIAGE_RETURN );	
						
				tmr0SerialPrint = 0;	
			}
		#endif
		
		
		// For low frequencies you want the display reading to persist instead of zeroing out.
		// Until the RPM threshold is exceeded, the display will not zero out.
		// A reboot will reset this parameter.
		if( totClockCycles > 0 && totClockCycles <= ZERO_DISPLAY_SPEED ){ doZeroDisplay = 1;} 
		
		if( tmr0ZeroDisplay >= ZERO_DISPLAY_TIME && doZeroDisplay == 1)
		{
			bubbleOutput = 0;
			serialOutput = 0;
			tmr0ZeroDisplay = 0;
			totClockCycles = 0;
			DP0 = 0;
			DP1 = 0;
			DP2 = 0;
			DP3 = 0;
			Drvr_GPIO_Led_Off();
		}
		
		// Go to sleep 
		#ifdef SLEEP_ENABLE
			
			if( switchState == 0 )
			{
				// Prepare to sleep
				bubbleOutput = 0;
				serialOutput = 0;
				tmr0ZeroDisplay = 0;	
			
				// Turn off IO pins
				Drvr_Tach_Rexmit_Off();
				Drvr_GPIO_Led_Off();
				Drvr_Tach_Sensor_Disable();
						
				// Turn off the bubble display shift register.
				shiftByteOut( 0b00000000 );
				
				// Disable the low RPM display zeroing threshold
				doZeroDisplay = 0;
			
			
				// Sleep
				sleep_enable();
				sleep_cpu();		
				// We will resume execution here after an external interrupt.
			
				// Wake up!
				sleep_disable();
				Drvr_Tach_Sensor_Enable();	// Turn on the IR pickup
				//switchPressed = 0;
			}
		#endif
		#endif
		
	}
}

ISR( TIMER0_OVF_vect )
{	
	/* Increment the system tick counter. */
    App_Scheduler_Bump_Sys_Tick();
}

ISR( INT0_vect )
{
    /* No code. Wake from sleep ISR. */	
}

