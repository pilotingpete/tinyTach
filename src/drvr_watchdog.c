#include "drvr_watchdog.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>

void Drvr_Watchdog_Init( void )
{
    /* Set the watchdog timeout period. */
    wdt_enable( WDTO_4S );

    /* Enable the watchdog. */
    WDTCR |= ( 1 << WDE );
}

void Drvr_Watchdog_Off( void )
{
    cli();

    Drvr_Watchdog_Pet();

    /* Clear WDRF in MCUSR */
    MCUSR = 0x00;
    /* Write logical one to WDCE and WDE */
    WDTCR |= ( ( 1 << WDCE ) | ( 1 << WDE ) );
    /* Turn off WDT */
    WDTCR = 0x00;

    sei();
}

void Drvr_Watchdog_Pet( void )
{
    wdt_reset();
}