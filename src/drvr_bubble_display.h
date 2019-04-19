#ifndef _HEADER_GUARD_UUID_85186272_CD42_4099_9731_6F9E184727B4
#define _HEADER_GUARD_UUID_85186272_CD42_4099_9731_6F9E184727B4

/** @file
 * Tiny Tach uses a single 4 character bubble display, the QDSP 6064.
 * The Display is driven thru a shift register for the segments
 * and GPIO for the cathodes. The display is multiplexed @ ~256Hz
 * The bubble display has 4 cathodes, one for each digit. The 
 * cathodes are connected to the uC pins and configured to sink current.
 * Set the the corresponding uC pin LOW to select a cathode to 
 * sink current and display data. 
 * Set the uC pin HIGH to deselect a cathode.
 */

/** A structure representing a digit to print on the bubble display.
*/
typedef struct
{
    uint8_t number;   /* The number to print. 0::9 */
    uint8_t location; /* The location of the digit. 
                         * 0 is leftmost, and 3 is rightmost.
                         */
    uint8_t decimal;  /* Set to 1 to display the decimal point.
                         * Set to 0 to show no decimal point.
                         */
} bubble_t;

/** Initialize the Bubble Display driver. This shoud be called from main() 
 * prior to using this module.
 */
extern void Drvr_Bubble_Display_Init(void);

/** Sends a number to the bubble display at the location specified.
 * This function should be called at a regular periodic interval
 * to support multiplexing. of the display.
 *
 * \param[in] number is the numeral to display 0::9.
 * \param[in] location is the position. 0 is the leftmost character
 * and 3 is the rightmost. i.e. "thousands" and "ones" respectively.
 * \param[in] do_decimal is set to 1 to display the decimal point on the 
 * particular digit. Set to 0 to not display the decimal point.
 */
extern void Drvr_Bubble_Display_Print(bubble_t *digit, uint8_t location);

/** Shifts out an empty byte to set the shift reg pins low.
 */
extern void Drvr_Bubble_Display_Shutdown(void);

#endif /* _HEADER_GUARD_UUID_85186272_CD42_4099_9731_6F9E184727B4 */