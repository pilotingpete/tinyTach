#ifndef _HEADER_GUARD_UUID_BBC4426F_EF65_43E4_9807_F4A1E3F5C496
#define _HEADER_GUARD_UUID_BBC4426F_EF65_43E4_9807_F4A1E3F5C496

/** Update the bubble display task. */
extern void Task_Bubble_Display( void );

/** Set the data to show on the bubble display.
 * 
 * \param *data pointer to the data to display
 * \param decimal
 */
extern void Task_Bubble_Display_Set_Bubble_Data( uint16_t *data, uint8_t decimal );

extern void Task_Bubble_Display_Set_Data_Hold( uint16_t *data );

extern void Task_Data_Hold( void );

#endif /* _HEADER_GUARD_UUID_BBC4426F_EF65_43E4_9807_F4A1E3F5C496 */