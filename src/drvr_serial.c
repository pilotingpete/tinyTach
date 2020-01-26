#include "drvr_serial.h"
#include <avr/io.h>

#define BaudRate 19200
#define MYUBRR ( ( F_CPU / 16 / BaudRate ) - 1 )

static uint8_t tx_ready( void )
{
    /* Nonzero if TX register is ready to receive new data. */
    return ( UCSRA & _BV( UDRE ) );
}

static void send_char( unsigned char data )
{
    while( tx_ready() == 0 ) /* While NOT ready. */
    {
        ;
        ;
    }
    UDR = data;
}

void Drvr_Serial_Init( void )
{
    /* Set the baud rate */
    UBRRH = ( uint8_t )( MYUBRR >> 8 );
    UBRRL = (uint8_t)MYUBRR;

    /* Enable receiver and transmitter. */
    UCSRB = ( 1 << RXEN ) | ( 1 << TXEN );

    /* Frame format: 8data, No parity, 1stop bit */
    UCSRC = ( 3 << UCSZ0 );
}

void Drvr_Serial_Print_String( const char *str )
{
    while( *str )
    {
        send_char( *str++ );
    }
}