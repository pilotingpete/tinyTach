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

/** Calculate the input capture period.
 *
 * \return a pointer to the input capture clock cycles.
 */
extern uint32_t *Drvr_Tach_Calc_Period( void );

/** Arm/rearm the input capture system.
 */
extern void Drvr_Tach_Arm_Input_Capture( void );

/** Getter for the capture state.
 *
 * \return capture_state
 * 0 -> no capture events registered.
 * 1 -> first capture event complete.
 * 2 -> second capture event complete. - ready for calculation.
 */
extern uint8_t Drvr_Tach_Get_Capture_State( void );

#endif /* _HEADER_GUARD_UUID_AF3C330E_A241_42AD_8917_C22867DA0FD3 */