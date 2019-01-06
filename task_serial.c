#include <stdlib.h>
#include <avr/io.h>
#include "task_serial.h"
#include "drvr_serial.h"

/* Serial output buffer. */
char buff[10]; 

void Task_Serial( void )
{
        Drvr_Serial_Print_String( "Serial Task\r\n" );
  
        //ultoa( 46622342, buff, 10 );
        //Drvr_Serial_Print_String( buff );

        //Drvr_Serial_Print_String( "\r\n" );

        //ultoa( 123, buff, 10 );
        //Drvr_Serial_Print_String( buff );

        //Drvr_Serial_Print_String( "\r\n" );
}