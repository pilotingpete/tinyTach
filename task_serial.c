#include <stdlib.h>
#include <avr/io.h>
#include "task_serial.h"
#include "drvr_serial.h"
#include "drvr_tach.h"

/* Serial output buffer. */
char buff[10]; 

void Task_Serial( void )
{
        Drvr_Serial_Print_String( "Frequency Hz: " );
  
        uint32_t freq = Drvr_Tach_Get_Freq();

		ultoa( freq, buff, 10 );
        Drvr_Serial_Print_String( buff );
        Drvr_Serial_Print_String( "\n\r" );

}