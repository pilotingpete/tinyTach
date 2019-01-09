#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>	
#include "drvr_tach.h"
#include "drvr_gpio.h"



/* Tach signal retransmit output */
#define	REXMIT_PORT PORTD
#define REXMIT      PD4	

/* IR emitter/detector pickup */
#define	IR_PORT    PORTD
#define IR_INPUT   PD6		
#define IR_ENABLE  PD3

#define FIRST_CAPTURE 			0
#define SECOND_CAPTURE			1

/* Min of (65536 / 394) * 60 = ~9980 DUT cycles per minute. 
 * i.e this limits the max "RPM" to guard against false 
 * positive triggers. 
 */
#define MIN_CLK_CYCLES    394		

static volatile uint16_t input_cap_1 = 0;
static volatile uint16_t input_cap_2 = 0;											
static volatile uint8_t  capture_state = 0;
static volatile uint8_t num_overflows_tmr_1 = 0;

static uint32_t total_clock_cycles = 0;

static void disable_tach_timer_interrupts( void )
{
	/* Disable timer capture and overflow interrupts */
    TIMSK &= ~( ( 1 << TOIE1 ) | ( 1 << ICIE1 ) );	
}

static void calc_clk_cyc( void )
{	
	const uint32_t ovf_val = 65536;

	cli(); /* Disable interrupts during this calculation. */
	total_clock_cycles = (uint32_t)input_cap_2 - (uint32_t)input_cap_1;
	total_clock_cycles += num_overflows_tmr_1 * ovf_val;
	sei(); /* Reenable global interrupts */
}

void Drvr_Tach_Init( void )
{
    /* Tach input IR detector */
	DDRD &= ~( 1 << IR_INPUT );
	/* Enable the internal pullup. */
	IR_PORT |= ( 1 << IR_INPUT );
	
	/* Tach enable - IR Emitter */
	/* Set the LED pin for output. */
	DDRD |= ( 1 << IR_ENABLE );
	/* Set the IR emitter pin high. */
	IR_PORT |= ( 1 << IR_ENABLE );

    /* Set the LED pin for output. */
	DDRD |=  ( 1 << REXMIT ) ;
	/* Clear the LED port bit to set the output low. */
	REXMIT_PORT &= ~( 1 << REXMIT );

    /* Timer 1 configuration. */
	TCCR1B |= ( 1 << ICNC1 );					/* Enable input capture noise canceler. */
	/*TCCR1B |= ( 1 << ICES1 );*/				/* Trigger on rising edge of ICP1 pin. */
	TCCR1B &= ~( 1 << ICES1 );					/* Trigger on falling edge of ICP1 pin. */
	TCCR1B |= (( 1 << CS11 ) | ( 1 << CS10 ));	/* Div64 on clock for 4.194304 MHz / 64 = 65536 */
	
	/* Interrupt enable */
	TIMSK |= ( 1 << TOIE1 );	/* Timer 1 overflow interrupt */
	TIMSK |= ( 1 << ICIE1 );	/* Input capture interrupt */
}

void Drvr_Tach_Rexmit_Off( void )
{
	REXMIT_PORT &= ~( 1 << REXMIT );
}

void Drvr_Tach_Rexmit_On( void )
{
	REXMIT_PORT |= ( 1 << REXMIT );
}

void Drvr_Tach_Sensor_Disable( void )
{
	IR_PORT &= ~( 1 << IR_ENABLE );
}

void Drvr_Tach_Sensor_Enable( void )
{
	IR_PORT |= ( 1 << IR_ENABLE );
}

uint8_t Drvr_Tach_Get_Capture_State( void )
{
	return capture_state;
}

void Drvr_Tach_Rearm_Input_Capture( void )
{
	input_cap_1 = input_cap_2;								
    capture_state = SECOND_CAPTURE;
    num_overflows_tmr_1 = 0;

    /* Enable timer overflow and input capture interrupts. */
	TIMSK |= ( ( 1 << TOIE1 ) | ( 1 << ICIE1 ) );
}

uint32_t Drvr_Tach_Get_Clk_Cyc( void )
{
	uint32_t check_cyc = 0;

	do
	{
        check_cyc = total_clock_cycles;
	}while( check_cyc != total_clock_cycles );

    return total_clock_cycles;
}

/* For extending the input capture timing capability. */
ISR( TIMER1_OVF_vect )
{
	/* Limit the overflows so this doesn't roll over too... */
	if( num_overflows_tmr_1 < 255 ) 
		num_overflows_tmr_1++;
}

/* Input capture period measurement. */
ISR( TIMER1_CAPT_vect )
{
	switch( capture_state )
	{
		case FIRST_CAPTURE:
		    /* First input capture time */
			input_cap_1 = ICR1;	
			capture_state++;
			break;
			
		case SECOND_CAPTURE:
		    /* Second input capture time */
			input_cap_2 = ICR1;	
			
			disable_tach_timer_interrupts();
			calc_clk_cyc();
			capture_state++;
			break;
		
		default:
			break;	
	}
}
