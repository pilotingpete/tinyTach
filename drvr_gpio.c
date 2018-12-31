#include <avr/io.h>
#include "drvr_gpio.h"

/* LED */
#define	LED_PORT    PORTD
#define LED         PD5

/* Tact switch */
#define	SW1_PORT    PORTD
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
	return bit_is_clear( SW1_PORT, SW1 );
}
