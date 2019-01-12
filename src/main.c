

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

const uint8_t min_switch_state = 0;
const uint8_t max_switch_state = 3;

static uint8_t new_data = 0;
static uint8_t switch_state = 1;
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
			Task_Bubble_Display_Set_Bubble_Data( (uintptr_t*)switch_state, 5 );
			Task_Bubble_Display_Set_Data_Hold( (uintptr_t*)10 );
			
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

	        /* Toggle the retransmit output pin.
	         * This output will toggle at half the intpuf frequency
	         */
	        Drvr_Retransmit_Toggle();
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
				Drvr_Tach_Reset();
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
                	/* Display Hz with xx.xx precision */
                	const uint16_t output = 100 * F_CPU / 64 / clock_cycles;
                    Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)output, 2 );     	
                }
                
            break;

            case 3: /* Display data as period - ms */

                if( new_data )
                {
                	uint16_t output = 0;

                    /* Autoscale the bubble display */
                	if( clock_cycles > 62500 )
                	{
                		/* Display in seconds with xx.xx precision. */
                        output = ( 100 * clock_cycles ) / ( F_CPU / 64 );
                        Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)output, 2 );
                	}
                	else
                	{
                		/* Display in miliseconds with xxx.x precision */
                        output = ( 10000 * clock_cycles ) / ( F_CPU / 64 );
                        Task_Bubble_Display_Set_Bubble_Data( (uint16_t*)output, 3 );
                    }
                }

            break;
  
            default:
                /* Not Expected */
            break;
        }

        new_data = 0;

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

