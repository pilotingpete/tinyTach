#ifndef _HEADER_GUARD_UUID_AE40C25E_F3F8_47E3_816F_94FEDCB665D0
#define _HEADER_GUARD_UUID_AE40C25E_F3F8_47E3_816F_94FEDCB665D0

/** @file
 * A driver for the watchdog timer.
 */

/** Initialize the watchdog timer. This shoud be called from main() 
 * prior to using this module.
 */
extern void Drvr_Watchdog_Init( void );

/** Disable the watchdog timer.
 */
extern void Drvr_Watchdog_Off( void );

/** Service the watchdog timer system to show we are alive still.
 */
extern void Drvr_Watchdog_Pet( void );

#endif /* _HEADER_GUARD_UUID_AE40C25E_F3F8_47E3_816F_94FEDCB665D0 */