#include <stdlib.h>
#include <avr/io.h>
#include <string.h>
#include "drvr_bubble_display.h"
#include "task_bubble_display.h"

static bubble_t bubble_data[4];
static uint16_t data_hold;

void Task_Bubble_Display( void )
{
	static uint8_t i;

	if( ++i > 3 )
	{
		i = 0;
	}
    Drvr_Bubble_Display_Print( &bubble_data[i], i );
}

void Task_Bubble_Display_Set_Data_Hold( uint16_t *data )
{
    data_hold = data;
}

void Task_Data_Hold( void )
{
	if( data_hold > 0 )
	{
		data_hold--;	
	}
}

void Task_Bubble_Display_Set_Bubble_Data( uint16_t *data, uint8_t decimal )
{
    if( !data_hold )
    {
	    /* Clear the bubble data structure */
		memset( bubble_data, 0, sizeof( bubble_t ) * 4);
	    
	    /* Place the decimal point. */
		bubble_data[decimal].decimal = 1;

		/* Thousands */
		bubble_data[0].number = ( (uint16_t)data % 10000 ) / 1000;
		bubble_data[0].location = 0;

	    /* Hundreds */
		bubble_data[1].number = ( (uint16_t)data % 1000 ) / 100;
		bubble_data[1].location = 1;
		
		/* Tens */
		bubble_data[2].number = ( (uint16_t)data % 100 ) / 10;
		bubble_data[2].location = 2;
		
		/* Ones */
		bubble_data[3].number = (uint16_t)data % 10;
		bubble_data[3].location = 3;
	}
}
