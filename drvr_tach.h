#ifndef _HEADER_GUARD_UUID_AF3C330E_A241_42AD_8917_C22867DA0FD3
#define _HEADER_GUARD_UUID_AF3C330E_A241_42AD_8917_C22867DA0FD3

/** @file
 * This module contains ISR config, GPIO setup, and calculations
 * related to the tachometer function.
 */

/** Initialize the tach driver. This shoud be called from main() 
 * prior to using this module.
 */
extern void Drvr_Tach_Init( void );

/** Turn OFF the retransmit pin.
 */
extern void Drvr_Tach_Rexmit_Off( void );

/** Turn ON the retransmit pin
 */
extern void Drvr_Tach_Rexmit_On( void );

/** Turn OFF the IR emitter pin.
 */
extern void Drvr_Tach_Sensor_Disable( void );

/** Turn ON the IR emitter pin
 */
extern void Drvr_Tach_Sensor_Enable( void );

#endif /* _HEADER_GUARD_UUID_AF3C330E_A241_42AD_8917_C22867DA0FD3 */