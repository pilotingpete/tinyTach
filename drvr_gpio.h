#ifndef _HEADER_GUARD_UUID_D5A11F17_E967_4EC6_90BD_AD8664F9389C
#define _HEADER_GUARD_UUID_D5A11F17_E967_4EC6_90BD_AD8664F9389C

/** @file
 * GPIO Driver
 */


/** Initialize the GPIO
 */
extern void Drvr_GPIO_Init( void );

/** Toggle the LED output.
 */
extern void Drvr_GPIO_Led_Toggle( void );

/** Turn Off the LED output.
 */
extern void Drvr_GPIO_Led_Off( void );

/** Turn On the LED output.
 */
extern void Drvr_GPIO_Led_On( void );

#endif /* _HEADER_GUARD_UUID_D5A11F17_E967_4EC6_90BD_AD8664F9389C */