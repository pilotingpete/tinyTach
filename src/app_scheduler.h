#ifndef _HEADER_GUARD_UUID_8C28487B_9769_4759_991E_3DBE1F49D232
#define _HEADER_GUARD_UUID_8C28487B_9769_4759_991E_3DBE1F49D232

/** @file
 * This is a simple tick time scheduler.
 */

typedef struct
{
    uint16_t delta;          //*< Timer interval to run task */
    uint32_t prev_tick;      //*< Last run tick time. */
    void ( *func )( void );  //*< Function pointer for the task to run. */
} task_t;

/** Run the scheduler task array.
 * The task array should have a NULL pointer reference for the function
 * in the last entry. This is how the scheduler knows it has reached
 * the end of the task list.
 *
 * \param[in] const task_t *tasks is a pointer to an array of tasks to run.
 */
extern void App_Scheduler_Run_Tasks( task_t *tasks );

/** Bump the sys tick timer. Basically, just put this function in a periodic
 * ISR function.
 */
extern void App_Scheduler_Bump_Sys_Tick( void );

/** Get the current value of the sys_tick counter.
 * 
 * Since a 32-bit variable reads are non-atomic, the static variable
 * is read twice to make it isn't being modified during a read.
 *
 * \return the 32-bit sys_tick counter value. 
*/
extern uint32_t App_Scheduler_Get_Sys_Tick( void );

#endif /* _HEADER_GUARD_UUID_8C28487B_9769_4759_991E_3DBE1F49D232 */