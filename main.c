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
#include "task_bubble_display.h"
#include "task_watchdog.h"

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

static char buff[10]; 

static uint8_t new_data = 0;

static uint8_t switch_state = 1;
const uint8_t min_switch_state = 0;
const uint8_t max_switch_state = 3;

static uint32_t clock_cycles = 0;

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
        { TICK__100MS , 0, Task_Data_Hold },
        { TICK__2000MS , 0, Task_Watchdog },
        //* { TICK__10MS , 0, Task_Bubble_Display }, */
        /* End of the task list. */
  	    { 0 , 0, NULL }
    };


	while(1)
	{	
		/* Scroll the input switch state. */
		if( Drvr_GPIO_Switch_Is_Pressed() )
		{
			
			if( ++switch_state > max_switch_state )
			{	
				switch_state = min_switch_state;
			}
			
			/* Show the new state on the bubble display for a while. */
			Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)switch_state, 5 );
			_delay_ms(500);//Task_Bubble_Display_Set_Data_Hold( (uint16_t*)10 );
			Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)0, 0 );
			
			Drvr_GPIO_Led_Off();
		}

		/* Run all the tasks in tasks[] */
		App_Scheduler_Run_Tasks( tasks );

		/* Get new input capture data if it is ready. */
		if( Drvr_Tach_Get_Capture_State() == CAPTURE_RESULT_READY )
		{
			clock_cycles = Drvr_Tach_Get_Clk_Cyc();

			new_data = 1;

			Drvr_Tach_Rearm_Input_Capture();

            /* Visual indicator of input signal "quality" */
	        Drvr_GPIO_Led_Toggle();
        }

        /* Convert the output format and resolution. 
         * Display the data value if new data has been delivered.
         */
        switch( switch_state ) 
        {  

            case 0: /* Go to sleep. */

                /* Prepare to sleep. */
				Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)0, 0 );	
			
				/* Turn off IO pins to save power. */
				Drvr_Tach_Rexmit_Off();
				Drvr_GPIO_Led_Off();
				Drvr_Tach_Sensor_Disable();
						
				/* Turn off the bubble display shift register. */
				Drvr_Bubble_Display_Shutdown();
			
				/* Sleep. */
				Drvr_Watchdog_Off();
				sleep_enable();
				sleep_cpu();

				/* Execution will resume here after an external interrupt
				 * from the onboard tact switch. 
				 */
			
				/* Wake up! */
				Drvr_Watchdog_Init();
				sleep_disable();
				/* Turn on the IR pickup. */
				Drvr_Tach_Sensor_Enable();	
                
            break;
	
            case 1: /* Display data in cycles per minute - cpm */
                
                if( new_data )
                {
                	const uint16_t output = ( F_CPU / 64 / clock_cycles ) * 60;
                    Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)output, 0 );     	
                }

            break;

            case 2: /* Display data in cycles per second - Hz */

                if( new_data )
                {
                	const uint16_t output = F_CPU / 64 / clock_cycles;
                    Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)output, 0 );     	
                }
                
            break;

            case 3: /* Display data as period - ms */

                if( new_data )
                {
                    const uint16_t output = ( 1000UL * (uint32_t)F_CPU ) / (uint32_t)clock_cycles;
                    Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)output, 0 );
                }

            break;
  
            default:
                /* Not Expected */
            break;
        }

        new_data = 0;



		#if 0	
		// If a new frequency is ready for calculation
		if( capture_state >= CAPTURE_RESULT_READY )	
		{
			
			// Calculate the total number of system clock cycles between tach input pulses
			if ( input_cap_2 < input_cap_1 )
			{
				input_cap_2 += 65536;
			}
			
			
			
			
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
    Task_Bubble_Display();
}

ISR( INT0_vect )
{
    /* No code. Wake from sleep ISR. */	
}

