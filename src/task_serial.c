#include <stdlib.h>
#include <avr/io.h>
#include "task_serial.h"
#include "drvr_serial.h"
#include "drvr_tach.h"

/* Serial output buffer. */
char buff[10]; 

void Task_Serial( void )
{
  
        uint32_t clk_cyc = Drvr_Tach_Get_Clk_Cyc();

        Drvr_Serial_Print_String( "Clock Cycles: " );
		ultoa( clk_cyc, buff, 10 );
        Drvr_Serial_Print_String( buff );
        Drvr_Serial_Print_String( "\n\r" );

}