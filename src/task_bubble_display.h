#ifndef _HEADER_GUARD_UUID_BBC4426F_EF65_43E4_9807_F4A1E3F5C496
#define _HEADER_GUARD_UUID_BBC4426F_EF65_43E4_9807_F4A1E3F5C496

/** Update the bubble display task. */
extern void Task_Bubble_Display(void);

/** Set the data to show on the bubble display.
 * 
 * \param *data pointer to the data to display
 * \param decimal - pass 0 for no decimal points, 1::4 for leftmost to rightmost
 * respectively, and 5 for all decimal points ON.
 */
extern void Task_Bubble_Display_Set_Bubble_Data(uint16_t *data, uint8_t decimal);

/** Setter for the data hold time.
 * The data on the display will be "frozen" on the bubble display for the calculated
 * duration. The duration = Task_Data_Hold() delta time * the *data param here.
 *
 * e.g. Task_Data_hold() delta = 100 ms and *data = 30, then the display will hold
 * it's display for 3 seconds.
 * 
 * \param *data is a pointer to the time to hold data on the display.
 */
extern void Task_Bubble_Display_Set_Data_Hold(uint16_t *data);

/** A countdown task to devrement the data hold display.
 */
extern void Task_Data_Hold(void);

#endif /* _HEADER_GUARD_UUID_BBC4426F_EF65_43E4_9807_F4A1E3F5C496 */