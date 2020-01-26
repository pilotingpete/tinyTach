#ifndef _HEADER_GUIAR_UUID_1E7C3751_0180_44FD_BB16_C48C0D5A74E7
#define _HEADER_GUIAR_UUID_1E7C3751_0180_44FD_BB16_C48C0D5A74E7

/** @file
 * This driver is for UART Serial. 
 * Only TX is configured right now. 
 */

/** Initialize the Serial driver. This shoud be called from main() 
 * prior to using this module.
 */
extern void Drvr_Serial_Init( void );

/** Prints a null terminated string to the serial uart
 *
 * \param[in] const char *str is a pointer to the string.
 */
extern void Drvr_Serial_Print_String( const char *str );

#endif /* _HEADER_GUIAR_UUID_1E7C3751_0180_44FD_BB16_C48C0D5A74E7 */