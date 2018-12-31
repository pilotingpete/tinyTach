#ifndef _HEADER_GUARD_UUID_D5A11F17_E967_4EC6_90BD_AD8664F9389C
#define _HEADER_GUARD_UUID_D5A11F17_E967_4EC6_90BD_AD8664F9389C

/** @file
 * This driver is intended to interface port pins for GPIO. 
 * Only basic GPIO such as an LED is handled here. 
 */

/** Initialize the GPIO driver. This shoud be called from main() 
 * prior to using this module.
 */
extern void Drvr_GPIO_Init( void );

/** Toggle the green board LED. Calling this function will turn 
 * ON the LED if it is OFF and vice versa.
 */
extern void Drvr_GPIO_Led_Toggle( void );

/** Turn OFF the green board LED. Calling this function when the
 * LED is OFF already will have no effect.
 */
extern void Drvr_GPIO_Led_Off( void );

/** Turn ON the green board LED. Calling this function when the 
 * LED is ON already will have no effect.
 */
extern void Drvr_GPIO_Led_On( void );

/** Function to interface the tactile switch onboard. 
 * \return 1 if the switch is pressed. Otherwise, return 0.
 */
extern uint8_t Drvr_GPIO_Switch_Is_Pressed( void );

#endif /* _HEADER_GUARD_UUID_D5A11F17_E967_4EC6_90BD_AD8664F9389C */