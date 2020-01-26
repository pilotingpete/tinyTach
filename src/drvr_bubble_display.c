#include "drvr_bubble_display.h"
#include <avr/io.h>

/* Shift register GPIO */
#define SHIFT_REG_PORT PORTB
#define RCLK PB4
#define SER PB5
#define SRCLK PB7

/* One cathode per display character. 4 digits, 4 cathodes.
 */
#define BUBBLE_CATH_PORT PORTB
#define CATH_0 PB0
#define CATH_1 PB1
#define CATH_2 PB2
#define CATH_3 PB3

/* Byte layout for shifting a digit to the bubble display. */
/*		Definition	Byte to shift 	Bubble Digit	Segments Required */
/*____________________________________________________________________*/
#define DIGIT_0 0b00111111 /*	0				A,B,C,D,E,F		  */
#define DIGIT_1 0b00000110 /*	1				B,C				  */
#define DIGIT_2 0b01011011 /*	2				A,B,D,E,G		  */
#define DIGIT_3 0b01001111 /*	3				A,B,C,D,G		  */
#define DIGIT_4 0b01100110 /*	4				B,C,F,G			  */
#define DIGIT_5 0b01101101 /* 	5				A,C,D,F,G		  */
#define DIGIT_6 0b01111101 /*	6				A,C,D,E,F,G		  */
#define DIGIT_7 0b00000111 /*	7				A,B,C			  */
#define DIGIT_8 0b01111111 /*	8	`			A,B,C,D,E,F,G	  */
#define DIGIT_9 0b01101111 /*	9				A,B,C,D,F,G		  */
#define DECIMAL 0b10000000 /* 	DP								  */

static void shift_reg_pulse( void )
{
    SHIFT_REG_PORT |= ( 1 << SRCLK );  /* Set the serial clock line HIGH.*/
    SHIFT_REG_PORT &= ~( 1 << SRCLK ); /* Set the serial Clock line LOW. */
}

/* Shifting data out on the 74HC595 shift register. */
static void shift_byte_out( uint8_t byte_to_shift )
{
    /* Hold low while shifting data. */
    SHIFT_REG_PORT &= ~( 1 << RCLK );

    /* Traverse the byte, high to low, and clock out data. */
    for( int8_t i = 7; i >= 0; i-- )
    {
        /* If the bit is high. */
        if( byte_to_shift & ( 1 << i ) )
        {
            SHIFT_REG_PORT |= ( 1 << SER ); /* Set the register pin high. */
        }
        else
        {
            SHIFT_REG_PORT &= ~( 1 << SER ); /* Set the register pin low. */
        }

        /* Clock that bit out */
        shift_reg_pulse();
    }

    /* Latch data to shift register output pins. */
    SHIFT_REG_PORT |= ( 1 << RCLK );
}

static void build_bubble_byte( uint8_t digit_to_send, uint8_t dp )
{
    uint8_t byte_to_shift = 0b00000000;

    switch( digit_to_send )
    {
        case 0:
            byte_to_shift = DIGIT_0;
            break;

        case 1:
            byte_to_shift = DIGIT_1;
            break;

        case 2:
            byte_to_shift = DIGIT_2;
            break;

        case 3:
            byte_to_shift = DIGIT_3;
            break;

        case 4:
            byte_to_shift = DIGIT_4;
            break;

        case 5:
            byte_to_shift = DIGIT_5;
            break;

        case 6:
            byte_to_shift = DIGIT_6;
            break;

        case 7:
            byte_to_shift = DIGIT_7;
            break;

        case 8:
            byte_to_shift = DIGIT_8;
            break;

        case 9:
            byte_to_shift = DIGIT_9;
            break;
    }

    if( dp )
    {
        /* Display the decimal point on this digit. */
        byte_to_shift |= DECIMAL;
    }

    shift_byte_out( byte_to_shift );
}

static void select_bubble_element( uint8_t ele )
{
    /* Lower nibble */
    const uint8_t mask = 0x0F;

    /* Set a dummy nibble HIGH for all elements OFF */
    uint8_t tmp = mask;

    /* The bubble element cathodes are tied to the uC pin.
     * Clear the bit to sink current and activate a 
     * bubble element. 
     */
    switch( ele )
    {
        case 0: /* Thousands position */
            tmp &= ~( 1 << CATH_0 );
            break;

        case 1: /* Hundreds */
            tmp &= ~( 1 << CATH_1 );
            break;

        case 2: /* Tens */
            tmp &= ~( 1 << CATH_2 );
            break;

        case 3: /* Ones */
            tmp &= ~( 1 << CATH_3 );
            break;

        default:
            /* No action */
            break;
    }

    /* Write the lower nibble to activate the desired cathode. */
    BUBBLE_CATH_PORT = ( BUBBLE_CATH_PORT & ~mask ) | ( tmp & mask );
}

void Drvr_Bubble_Display_Init( void )
{
    /* Shift register.
     * Set the bit for output. 
     */
    DDRB |= ( ( 1 << RCLK ) | ( 1 << SER ) | ( 1 << SRCLK ) );
    /* Set the outputs LOW. */
    SHIFT_REG_PORT &= ~( ( 1 << RCLK ) | ( 1 << SER ) | ( 1 << SRCLK ) );

    /* Bubble display cathode sinks. 
     * Set the bit for output. 
     */
    DDRB |= ( ( 1 << CATH_0 ) | ( 1 << CATH_1 ) | ( 1 << CATH_2 ) | ( 1 << CATH_3 ) );
    /* Set the outputs HIGH to not sink current */
    BUBBLE_CATH_PORT |= ( ( 1 << CATH_0 ) | ( 1 << CATH_1 ) | ( 1 << CATH_2 ) | ( 1 << CATH_3 ) );
}

void Drvr_Bubble_Display_Print( bubble_t *digit, uint8_t location )
{
    /* Send the new digit out on the shift register. */
    build_bubble_byte( digit->number, digit->decimal );
    /* Select which bubble element to display on. */
    select_bubble_element( location );
}

void Drvr_Bubble_Display_Shutdown( void )
{
    shift_byte_out( 0b00000000 );
    BUBBLE_CATH_PORT |= ( ( 1 << CATH_0 ) | ( 1 << CATH_1 ) | ( 1 << CATH_2 ) | ( 1 << CATH_3 ) );
}
