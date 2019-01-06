#include <avr/io.h>
#include <stdlib.h>
#include "app_scheduler.h"

/* The main system tick counter for running tasks. */
static volatile uint32_t sys_tick = 0;

void App_Scheduler_Run_Tasks( task_t *tasks )
{
	uint8_t i = 0;
    while( tasks[i].func != NULL )
    {
    	/* If enough time has passed since the last call, run the function again. */
    	if( ( App_Scheduler_Get_Sys_Tick() - tasks[i].prev_tick ) >= (uint32_t)tasks[i].delta )
    	{
    	    tasks[i].func();
    	    tasks[i].prev_tick = App_Scheduler_Get_Sys_Tick();
    	}

    	i++;
    }
}

void App_Scheduler_Bump_Sys_Tick( void )
{
	++sys_tick;
}

uint32_t App_Scheduler_Get_Sys_Tick( void )
{
	/* Since a 32-bit variable reads are non-atomic
	 * read twice to make sure sys_tick isn't being
	 * modified during a read.
	 */
	uint32_t check_tick;

	do
	{
        check_tick = sys_tick;
	}while( check_tick != sys_tick );

	return check_tick;
}