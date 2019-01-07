#include <avr/io.h>
#include <util/delay.h>	
#include "drvr_gpio.h"

/* LED */
#define	LED_PORT    PORTD
#define LED         PD5

/* Tact switch */
#define	SW1_PORT    PORTD
#define SW1_INPUT   PIND
#define SW1         PD2	

		

void Drvr_GPIO_Init( void )
{
	/* Set the LED pin for output. */
    DDRD |=  ( 1 << LED );
    /* Clear the LED port bit to set the output low. */
    LED_PORT &= ~( 1 << LED );

    /* Clear the switch pin for input. */
	DDRD &= ~( 1 << SW1 );
	/* Enable the internal pullup. */
	SW1_PORT |= ( 1 << SW1 );

	/* External interrupt. This allows the pushbutton to bring the tach out of 
     * the power down sleep mode 
     */
    MCUCR |= ( 1 << ISC01 );	/* The falling edge of INT0 generates an interrupt request. */
    GIMSK |= ( 1 << INT0 );		/* External Interrupt Request 0 Enable. */
}

void Drvr_GPIO_Led_Toggle( void )
{
	LED_PORT ^= ( 1 << LED );
}

void Drvr_GPIO_Led_Off( void )
{
	LED_PORT &= ~( 1 << LED );
}

void Drvr_GPIO_Led_On( void )
{
	LED_PORT |= ( 1 << LED );
}

uint8_t Drvr_GPIO_Switch_Is_Pressed( void )
{
	const uint8_t bounce_num = 40;
	const uint8_t bounce_ms = 1;
	uint8_t retval = 0;
	uint8_t count = 0;

	while( bit_is_clear( SW1_INPUT, SW1 ) )
	{
		if( count++ >= bounce_num )
		{
			retval = 1;
		}
		_delay_ms( bounce_ms );
	}
	return retval;
}