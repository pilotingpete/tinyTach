#ifndef _HEADER_GUARD_UUID_AF3C330E_A241_42AD_8917_C22867DA0FD3
#define _HEADER_GUARD_UUID_AF3C330E_A241_42AD_8917_C22867DA0FD3

/** @file
 * This module contains ISR config, GPIO setup, and calculations
 * related to the tachometer function.
 */

#define CAPTURE_RESULT_READY	2

/** Initialize the tach driver. This shoud be called from main() 
 * prior to using this module.
 */
extern void Drvr_Tach_Init( void );

/** Reinitialize tach counters to zero.
 */
extern void Drvr_Tach_Reset( void );

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

/** Get the total clock cycles.
 *
 * Since a multi-byte variable reads are non-atomic, the static variable
 * is read twice to make it isn't being modified during a read.
 *
 * \return the number of clock cycles between input capture events.
 */
extern uint32_t Drvr_Tach_Get_Clk_Cyc( void );

/** Rearm the input capture system.
 * This prepares the input capture system for the next input pulse.
 */
extern void Drvr_Tach_Rearm_Input_Capture( void );

/** Getter for the capture state.
 *
 * \return capture_state
 * 0 -> no capture events registered.
 * 1 -> first capture event complete.
 * 2 -> second capture event complete. - ready for calculation.
 */
extern uint8_t Drvr_Tach_Get_Capture_State( void );

#endif /* _HEADER_GUARD_UUID_AF3C330E_A241_42AD_8917_C22867DA0FD3 */