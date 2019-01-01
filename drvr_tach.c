#include <avr/io.h>
#include <avr/interrupt.h>	

/* Tach signal retransmit output */
#define	REXMIT_PORT PORTD
#define REXMIT      PD4	

/* IR emitter/detector pickup */
#define	IR_PORT    PORTD
#define IR_INPUT   PD6		
#define IR_ENABLE  PD3

#define FIRST_CAPTURE 			0
#define SECOND_CAPTURE			1


volatile uint8_t tmr1NumOverflows = 0;
volatile uint16_t tmr0ZeroDisplay = 0;
volatile uint32_t inputCap1 = 0;
volatile uint32_t inputCap2 = 0;
volatile uint8_t captureState = 0;



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
	TCCR1B |= ( 1 << ICNC1 );					/* Enable input capture noise canceler. *.
	/*TCCR1B |= ( 1 << ICES1 );*/					/* Trigger on rising edge of ICP1 pin. */
	TCCR1B &= ~( 1 << ICES1 );					/* Trigger on falling edge of ICP1 pin. */
	TCCR1B |= (( 1 << CS11 ) | ( 1 << CS10 ));	/* Div64 on clock for 4.194304 MHz / 64 = 65536 */
	
	/* Interrupt enable */
	TIMSK |= ( 1 << TOIE1 );	/* Timer 1 overflow interrupt */
	TIMSK |= ( 1 << ICIE1 );	/* Input capture interrupt */

    /* External interrupt. This allows the pushbutton to bring the tach out of 
     * the power down sleep mode 
     */
    MCUCR |= ( 1 << ISC11 );	/* The falling edge of INT1 generates an interrupt request. */
    GIMSK |= ( 1 << INT0 );		/* External Interrupt Request 0 Enable. */
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
	if( tmr1NumOverflows < 255 ) /* Limit the overflows so this doesn't roll over too... */
		tmr1NumOverflows++;
}

/* Input capture period measurement. */
ISR( TIMER1_CAPT_vect )
{
	switch( captureState )
	{
		case FIRST_CAPTURE:
			inputCap1 = ICR1;	/* First input capture time */
			captureState++;
			break;
			
		case SECOND_CAPTURE: 
			inputCap2 = ICR1;	/* Second input capture time */
			
			if( inputCap2 - inputCap1 > MIN_CLK_CYCLES ) {
				TIMSK &= ~( ( 1 << TOIE1 ) | ( 1 << ICIE1 ) );	// Disable timer capture and overflow interrupts
				captureState++;
			}
			break;
		
		default:
			break;	
	}
}
