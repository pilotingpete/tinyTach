#include <avr/io.h>
#include "drvr_gpio.h"

/* LED Configuration */
#define	LED_PORT    PORTD
#define LED         PD5


void Drvr_GPIO_Init( void )
{
	/* Set the bit for output. */
    DDRD |=  ( 1 << LED );
    
    /* Clear the LED port bit to set the output low. */
    LED_PORT &= ~( 1 << LED );
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
