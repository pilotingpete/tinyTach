#include <avr/io.h>
#include <avr/interrupt.h>	

/* Tach signal retransmit output */
#define	REXMIT_PORT PORTD
#define REXMIT      PD4	

/* IR emitter/detector pickup */
#define	IR_PORT    PORTD
#define IR_INPUT   PD6		
#define IR_ENABLE  PD3

/* Min of (65536 / 394) * 60 = ~9980 DUT cycles per minute. 
 * i.e this limits the max "RPM" to guard against false 
 * positive triggers. 
 */
#define MIN_CLK_CYCLES    394		

static volatile uint16_t input_cap_1 = 0;
static volatile uint16_t input_cap_2 = 0;											
static volatile uint8_t  capture_state = 0;
static volatile uint32_t num_overflows_tmr_1 = 0;

static void disable_tach_timer_interrupts( void )
{
	/* Disable timer capture and overflow interrupts */
    TIMSK &= ~( ( 1 << TOIE1 ) | ( 1 << ICIE1 ) );	
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

    /* External interrupt. This allows the pushbutton to bring the tach out of 
     * the power down sleep mode 
     */
    MCUCR |= ( 1 << ISC11 );	/* The falling edge of INT1 generates an interrupt request. */
//    GIMSK |= ( 1 << INT0 );		/* External Interrupt Request 0 Enable. */
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


/* For extending the input capture timing capability. */
ISR( TIMER1_OVF_vect )
{
	if( num_overflows_tmr_1 < 255 ) /* Limit the overflows so this doesn't roll over too... */
		num_overflows_tmr_1++;
}

/* Input capture period measurement. */
ISR( TIMER1_CAPT_vect )
{
	switch( capture_state )
	{
		case 1:
		    /* First input capture time */
			input_cap_1 = ICR1;	
			capture_state++;
			break;
			
		case 2:
		    /* Second input capture time */
			input_cap_2 = ICR1;	
			
			if( input_cap_2 - input_cap_1 > MIN_CLK_CYCLES ) {
				disable_tach_timer_interrupts();
				capture_state++;
			}
			break;
		
		default:
			break;	
	}
}
