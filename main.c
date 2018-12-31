/*

Date:	2016.11.22
Author:	Pete Mills

uses a single index mark, i.e. 1 ppr

*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~* Display States *~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
Pressing the input switch SW1 will cycle thru the available display units.

When SLEEP_ENABLE is defined, switchState = 0 puts tinyTach to sleep. The processor will wake up when 
SW1 is pressed again.

switchState		Bubble display / Serial output units
-----------		------------------------------------
0				Sleeps the processor until a button press wakes it up. - Only #ifdef SLEEP_ENABLE
1				Cycles per minute ( Cpm )
2				Cycles per second ( Hz )	
3				Period - Time between input pulses ( mS )		

*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~* Display States *~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*




*/

//~~~~~~~~~~~~~~~~~~~~~~******************** Includes ********************~~~~~~~~~~~~~~~~~~~~~~~~~

#include <avr/io.h>
#include <avr/interrupt.h>							
#include <util/delay.h>		
#include <avr/sleep.h>
#include <stdlib.h>
#include "main.h"
#include "drvr_gpio.h"
#include "drvr_bubble_display.h"
//~~~~~~~~~~~~~~~~~~~~~~******************** Includes ********************~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~************** Pre-Processor Directives *************~~~~~~~~~~~~~~~~~~~~~~~
#define SLEEP_ENABLE		// Hardware changes are necessary to make sleep modes useful.
#define SERIAL_ENABLE		// Also disables period output
#define PSTRETCH_ENABLE		// Pulse stretch output
//~~~~~~~~~~~~~~~~~~~~~************** Pre-Processor Directives *************~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~******************** Definitions ********************~~~~~~~~~~~~~~~~~~~~~~~

// Definitions



// retransmit
#define	REXMIT_PORT 		PORTD
#define REXMIT				PD4		

// non-contact pickup
#define	IR_PORT 			PORTD
#define IR_INPUT 			PD6		
#define IR_ENABLE			PD3



#define LINE_FEED		10
#define CARRIAGE_RETURN	13

#define BaudRate 9600
#define MYUBRR ((F_CPU / 16 / BaudRate ) - 1 )

#define FIRST_CAPTURE 			0
#define SECOND_CAPTURE			1
#define CAPTURE_RESULT_READY	2

#define MIN_CLK_CYCLES  	100//394		// min of (65536 / 394) * 60 = ~9980 DUT cycles per minute. i.e we are limiting the max "RPM" to guard 
											// against false positive triggers 

#define ARRAY_SIZE 			15		// moving average filter
#define CYCLE_FILTER_LIM	6554	// 6554 clock cycles ~= 600 RPM, Don't filter speed data if slower 

#define ZERO_DISPLAY_SPEED	3932	// 3932 clock cycles ~= 1000 RPM
									// Don't zero the display after a timeout if totClockCycles is less ( speed is faster )  
									// DO zero the display if you have never hit this threshold and a timeout has expired. 



// display update rates
#define TEN_SECONDS 	2550
#define ONE_SECOND 		255	// number of timer0 overflows in one second
#define HALF_SECOND		127
#define QUARTER_SECOND	63

#define SERIAL_SEND 	HALF_SECOND

#define PULSE_STRETCH		1		// one timer0 overflow is ~= 4mS duration for retransmitting the PPR
#define ZERO_DISPLAY_TIME	1280 	// timer0 overflows.

#define CLK2CPM 		3932160	// Numerator for converting clock cycles to cycles per minute. ( 65536 * 60 ) 
#define CLK2HZ			65536	// Numerator for converting clock cycles to cycles per second. ( 65536 * 1 )

#ifdef SLEEP_ENABLE
	#define MIN_SWITCHSTATE 0
#else
	#define MIN_SWITCHSTATE 1
#endif

#define MAX_SWITCHSTATE 3



//~~~~~~~~~~~~~~~~~~~~~******************** Definitions ********************~~~~~~~~~~~~~~~~~~~~~~~







//~~~~~~~~~~~~~~~~~~~******************** Global Variables ********************~~~~~~~~~~~~~~~~~~~~

volatile uint16_t bubbleOutput = 0;	// Sending data to the bubble display
uint32_t serialOutput = 0;			// Sending data to the UART
uint16_t cycPerMin = 0;				// DUT cycles per minute i.e. RPM
uint16_t cycPerSec = 0;				// DUT cycles per second i.e. Hz
uint32_t totClockCycles = 0;		// Clock cycles per part cycle i.e time per revolution

uint16_t filterArray[ ARRAY_SIZE ];

uint8_t switchState = 1;


volatile uint8_t tmr1NumOverflows = 0;
volatile uint16_t tmr0ZeroDisplay = 0;
volatile uint32_t inputCap1 = 0;
volatile uint32_t inputCap2 = 0;
volatile uint8_t captureState = 0;

#ifdef PSTRETCH_ENABLE
volatile uint8_t tmr0PulseStretch = 0;
#endif

#ifdef SERIAL_ENABLE
volatile uint8_t tmr0SerialPrint = 0;
#endif

volatile uint8_t DP0 = 0;
volatile uint8_t DP1 = 0;
volatile uint8_t DP2 = 0;
volatile uint8_t DP3 = 0;

uint8_t doZeroDisplay = 0;

//~~~~~~~~~~~~~~~~~~~******************** Global Variables ********************~~~~~~~~~~~~~~~~~~~~




//~~~~~~~~~~~~~~~~~~~~~~~~~******************** MAIN ********************~~~~~~~~~~~~~~~~~~~~~~~~~~
int main(void)
{
	Drvr_GPIO_Init();
	Drvr_Bubble_Display_Init();

ioInit();
timerInit();

#ifdef SERIAL_ENABLE
	uartInit();
#endif

#ifdef SLEEP_ENABLE
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
#endif

sei();


	while(1)
	{	
		bubble_t digit;

		digit = (bubble_t){.number = 0, .location = 0, .decimal = 1};
        Drvr_Bubble_Display_Print( digit );
        _delay_ms( 1000 );

        digit = (bubble_t){.number = 1, .location = 1, .decimal = 1};
        Drvr_Bubble_Display_Print( digit );
        _delay_ms( 1000 );

        digit = (bubble_t){.number = 2, .location = 2, .decimal = 1};
        Drvr_Bubble_Display_Print( digit );
        _delay_ms( 1000 );

        digit = (bubble_t){.number = 3, .location = 3, .decimal = 1};
        Drvr_Bubble_Display_Print( digit );
        _delay_ms( 1000 );

		#if 0
		// Input switch state
		if( bit_is_clear(PIND, SW1 ) )
		{
			switchState++;
			
			if( switchState > MAX_SWITCHSTATE )
			{	
				switchState = MIN_SWITCHSTATE;
			}
			
			//sendDigitToBubble( switchState, 0 );
			// Send the current state to the bubble display and serial if enabled
			// Turn on all the decimal points for visual effect and parsing when post processing data
			DP0 = 1;
			DP1 = 1;
			DP2 = 1;
			DP3 = 1;
			
			bubbleOutput = switchState;	// Temporarily display the switchState on the bubble display.
			
			_delay_ms(1000);		
			
			// Clear display
			bubbleOutput = 0;
			
			// Decimal points OFF
			DP0 = 0;
			DP1 = 0;
			DP2 = 0;
			DP3 = 0;
			
			Drvr_GPIO_Led_Off();
		}
		
		// If a new frequency is ready for calculation
		if( captureState >= CAPTURE_RESULT_READY )	
		{
			
			// Calculate the total number of system clock cycles between tach input pulses
			if ( inputCap2 < inputCap1 )
			{
				inputCap2 += 65536;
			}
			
			totClockCycles = inputCap2 - inputCap1;
			totClockCycles += tmr1NumOverflows * 65536;
			
			
			// Prepare the data for outputting in the correct units.
			switch( switchState )
			{
				case 1:	// Cycles per minute ( CPM/RPM )
					/*
					// Extend the low range resolution. i.e where RPM < 1
					if( totClockCycles > CLK2CPM ){
						totClockCycles /= 1000; dont modify totClockcycles
						DP0 = 1;	// turn on the decimal point to show < 1 CPM.
					}else{
						DP0 = 0;
						DP1 = 0;
					}
					*/
					cycPerMin = CLK2CPM / totClockCycles; 	
					
					bubbleOutput = cycPerMin;
						
				break;
				
				case 2: // Cycles per Second ( Hz )
					
					// Extend the low range resolution. i.e where Hz < 1
					if( totClockCycles > CLK2HZ ){
						cycPerSec = CLK2HZ / totClockCycles / 1000;	// Hz
						DP0 = 1;	// turn on the decimal point to show < 1 CPM.
					}else{
						cycPerSec = CLK2HZ / totClockCycles;	// Hz
						DP0 = 0;
						DP1 = 0;
					}
					
					bubbleOutput = cycPerSec;
				
				break;
				
				case 3: // Period ( mS )
					bubbleOutput = ( totClockCycles * 1000 ) / 65536;
				break;
				
				
			}
			
			
			// If the reading is 10000 or above, shift to the right by one digit and drop the 
			// last digit. Also, light up the second decimal points so you know the reading is actually * 10
			if ( bubbleOutput > 9999 ){
				bubbleOutput /= 10;
				DP1 = 1;
			}else{
				DP1 = 0;	
			}
			
				
			
			// Use the moving average filter for the reading if the rate is high enough.
			if( totClockCycles > 0 && totClockCycles < CYCLE_FILTER_LIM )
			{
				bubbleOutput = filter( bubbleOutput );
			}else{
				filter( bubbleOutput );
			}
			
			// Set a separate serial output variable as it needs to be formatted differently.
			serialOutput = bubbleOutput;
			
			// we got that first cycle time, now we need to grab the subsequent ones
			inputCap1 = inputCap2;						// the timer is still running, so we prepare for the next calculation 
			captureState = 1;							// Prepare the capture state machine
			tmr1NumOverflows = 0;
			TIMSK |= ( ( 1 << TOIE1 ) | ( 1 << ICIE1 ) );	// Enable timer capture and overflow interrupts
			
			
			// for the pulse stretch
			#ifdef PSTRETCH_ENABLE
				tmr0PulseStretch = 0; 
				REXMIT_PORT |= ( 1 << REXMIT );
			#endif
			
			// To visually check that we are getting a good tach pulse train.
			Drvr_GPIO_Led_Toggle();
			
			// Dont zero out the display; we have data!
			tmr0ZeroDisplay = 0;
		}
		

		
		
		// Turn off the retransmit bit if the ppr has been stretched enough
		#ifdef PSTRETCH_ENABLE
			if( tmr0PulseStretch >= PULSE_STRETCH )
			{
				REXMIT_PORT &= ~( 1 << REXMIT );	
			}
		#endif
		
		// Serial print
		#ifdef SERIAL_ENABLE
			if( tmr0SerialPrint >= SERIAL_SEND )
			{
				uartPrint( serialOutput, 0 );	// value
				serialWrite( 44 );				// ASCII Comma
				
				// print units
				switch(switchState)
				{
					case 1: // counts per min / RPM
						serialWrite( 67 );	// ASCII C
						serialWrite( 80 );	// ASCII P
						serialWrite( 77 );	// ASCII M
					break;
					
					case 2:	// Hz
						serialWrite( 72 );	// ASCII H
						serialWrite( 122 );	// ASCII z
					break;
					
					case 3:	// Period
						serialWrite( 109 );	// ASCII m
						serialWrite( 83 );	// ASCII S
					break;
				}
				
				serialWrite( 44 );				// ASCII Comma
				
				uartPrint( totClockCycles, 0 );	// Raw clock cycles
				
				serialWrite( 44 );	// ASCII Comma
				serialWrite( 67 );	// ASCII C
				serialWrite( 76 );	// ASCII L
				serialWrite( 75 );	// ASCII K
				
				serialWrite( LINE_FEED );
    			serialWrite( CARRIAGE_RETURN );	
						
				tmr0SerialPrint = 0;	
			}
		#endif
		
		
		// For low frequencies you want the display reading to persist instead of zeroing out.
		// Until the RPM threshold is exceeded, the display will not zero out.
		// A reboot will reset this parameter.
		if( totClockCycles > 0 && totClockCycles <= ZERO_DISPLAY_SPEED ){ doZeroDisplay = 1;} 
		
		if( tmr0ZeroDisplay >= ZERO_DISPLAY_TIME && doZeroDisplay == 1)
		{
			bubbleOutput = 0;
			serialOutput = 0;
			tmr0ZeroDisplay = 0;
			totClockCycles = 0;
			DP0 = 0;
			DP1 = 0;
			DP2 = 0;
			DP3 = 0;
			Drvr_GPIO_Led_Off();
		}
		
		// Go to sleep 
		#ifdef SLEEP_ENABLE
			
			if( switchState == 0 )
			{
				// Prepare to sleep
				bubbleOutput = 0;
				serialOutput = 0;
				tmr0ZeroDisplay = 0;	
			
				// Turn off IO pins
				REXMIT_PORT &= ~( 1 << REXMIT );
				Drvr_GPIO_Led_Off();
				IR_PORT &= ~( 1 << IR_ENABLE );	
						
				// Turn off the bubble display shift register.
				shiftByteOut( 0b00000000 );
				
				// Disable the low RPM display zeroing threshold
				doZeroDisplay = 0;
			
			
				// Sleep
				sleep_enable();
				sleep_cpu();		
				// We will resume execution here after an external interrupt.
			
				// Wake up!
				sleep_disable();
				IR_PORT |= ( 1 << IR_ENABLE );	// Turn on the IR pickup
				//switchPressed = 0;
			}
		#endif
		#endif
		
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~******************** MAIN ********************~~~~~~~~~~~~~~~~~~~~~~~~~~






void ioInit(void)
{
//~~~~~~~~~~~~~~~~~~******************** Pin Configuration ********************~~~~~~~~~~~~~~~~~~~~
	
    // Tach input
	DDRD &= ~( 1 << IR_INPUT );
	IR_PORT |= ( 1 << IR_INPUT );	// enable internal pullup
	
	// Tach enable
	DDRD |= ( 1 << IR_ENABLE ); 		// Set to "1" for output
	IR_PORT |= ( 1 << IR_ENABLE );	// Turn ON the IR pickup
	

	

	// Retransmit
	#ifdef PSTRETCH_ENABLE
		DDRD |=  ( 1 << REXMIT ) ;			// set pin to "1" for output 
		REXMIT_PORT &= ~( 1 << REXMIT  );	// set the outputs low
	#endif
//~~~~~~~~~~~~~~~~~~******************** Pin Configuration ********************~~~~~~~~~~~~~~~~~~~~
}
	
#ifdef SERIAL_ENABLE
void uartInit(void)
{	
    UBRRH = (unsigned char)( MYUBRR >> 8);    /* Set the baud rate */
    UBRRL = (unsigned char) MYUBRR; 

    UCSRB = (1<<RXEN)|(1<<TXEN);    /* Enable receiver and transmitter. */

    UCSRC = (3<<UCSZ0);    /* Frame format: 8data, No parity, 1stop bit */
}

#endif

void timerInit(void)
{
//~~~~~~~~~~~~~~~~~~******************* Timer0 Configuration ******************~~~~~~~~~~~~~~~~~~~~	
	TCCR0B |= ( ( 1 << CS01 ) | ( 1 << CS00 ) );	// Div64 on clock for 4.194304 MHz / 64 = 65536 Hz, 
													// so 256 8-bit timer overflows per second 
	
	TIMSK |= ( 1 << TOIE0 );	// Enable timer 0 overflow interrupt
//~~~~~~~~~~~~~~~~~~******************* Timer0 Configuration ******************~~~~~~~~~~~~~~~~~~~~	

//~~~~~~~~~~~~~~~~~~******************* Timer1 Configuration ******************~~~~~~~~~~~~~~~~~~~~	
	TCCR1B |= ( 1 << ICNC1 );					// Enable input capture noise canceler
	//TCCR1B |= ( 1 << ICES1 );					// Trigger on rising edge of ICP1 pin
	TCCR1B &= ~( 1 << ICES1 );					// Trigger on falling edge of ICP1 pin
	TCCR1B |= (( 1 << CS11 ) | ( 1 << CS10 ));	// Div64 on clock for 4.194304 MHz / 64 = 65536
	
	TIMSK |= ( 1 << TOIE1 );	// Enable timer 1 overflow interrupt
	TIMSK |= ( 1 << ICIE1 );	// Enable input capture interrupt
//~~~~~~~~~~~~~~~~~~******************* Timer1 Configuration ******************~~~~~~~~~~~~~~~~~~~~	

//~~~~~~~~~~~~~~~~~~*************** External INT0 Configuration ***************~~~~~~~~~~~~~~~~~~~~	
	#ifdef SLEEP_ENABLE
		MCUCR |= ( 1 << ISC11 );	// The falling edge of INT1 generates an interrupt request.
		GIMSK |= ( 1 << INT0 );		// External Interrupt Request 0 Enable
	#endif 
//~~~~~~~~~~~~~~~~~~*************** External INT0 Configuration ***************~~~~~~~~~~~~~~~~~~~~	


}



//~~~~~~~~~~~~~~~~~~~~~~******************** Functions ********************~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef SERIAL_ENABLE
// uart print Cycles Per minute
void uartPrint( uint32_t data , uint8_t newline)
{
	
	serialWrite( ( data / 10000000000 ) | 0x30 );
	serialWrite( ( ( data % 1000000000 ) / 100000000 ) | 0x30 );	
	serialWrite( ( ( data % 100000000 ) / 10000000 ) | 0x30 );
	serialWrite( ( ( data % 10000000 ) / 1000000 ) | 0x30 );	
	serialWrite( ( ( data % 1000000 ) / 100000 ) | 0x30 );	
	serialWrite( ( ( data % 100000 ) / 10000 ) | 0x30 );	
	
	if( DP0 ){
		serialWrite( 46 );	// decimal point		
	}else{
		serialWrite( ( ( data % 10000 ) / 1000 ) | 0x30 );	// send the thousands position OR-ed for decimal to ASCII conversion.
	}
	serialWrite( ( ( data % 1000 ) / 100 ) | 0x30 );	// hundreds 
	serialWrite( ( ( data % 100 ) / 10 ) | 0x30 );		// tens 
	serialWrite( ( data % 10 ) | 0x30 );				// ones 
	
	if ( newline )
	{
		serialWrite( LINE_FEED );
    	serialWrite( CARRIAGE_RETURN );	
    }
}
#endif

// For updating the display
// 256 Hz
ISR( TIMER0_OVF_vect )
{	
	#ifdef PSTRETCH_ENABLE
		tmr0PulseStretch++;	
	#endif
	
	#ifdef SERIAL_ENABLE
		tmr0SerialPrint++;
	#endif
	
	tmr0ZeroDisplay++;
	
	updateBubbleDisplay();
}

// For extending the input capture timing capability
ISR( TIMER1_OVF_vect )
{
	if( tmr1NumOverflows < 255 )	// Limit the overflows so this doesn't roll over too...
		tmr1NumOverflows++;
}

// input capture period measurement
ISR( TIMER1_CAPT_vect )
{
	switch( captureState )
	{
		case FIRST_CAPTURE:
			inputCap1 = ICR1;	// First input capture time
			captureState++;
			break;
			
		case SECOND_CAPTURE: 
			inputCap2 = ICR1;	// Second input capture time
			
			if( abs( inputCap2 - inputCap1 ) > MIN_CLK_CYCLES ) {
				TIMSK &= ~( ( 1 << TOIE1 ) | ( 1 << ICIE1 ) );	// Disable timer capture and overflow interrupts
				captureState++;
			}
			break;
		
		default:
			break;	
	}
}

#ifdef SLEEP_ENABLE
// External Interrupt
ISR( INT0_vect )
{	
}
#endif


uint16_t filter( uint16_t input ) 
{
	// FIR
	uint32_t arraySum = 0;
	
	for( int i = 0; i < ARRAY_SIZE - 1; i++ )
	{
		filterArray[ i ] = filterArray[ i + 1 ];
		arraySum += filterArray[ i ];
	}
	
	filterArray[ ARRAY_SIZE - 1 ] = input;
	arraySum += input;	
		
	return  arraySum / ARRAY_SIZE;   
	
}


//todo make task
void updateBubbleDisplay( void )
{
	static uint8_t myCathode;
	
	// The bubble display cathodes are connected to the AVR pins to sink current
	// Set the AVR pin high to turn OFF the bubble display cathode.
	// Set the AVR pin low to turn ON the bubble display cathode and sink current.
#if 0
	switch (myCathode)
	{	
	
		case 0:	// thousands position
			BUBBLE_CATH_PORT &= ~( 1 << CATH_0 );	
			//BUBBLE_CATH_PORT |= ( 1 << CATH_1 );	
			//BUBBLE_CATH_PORT |= ( 1 << CATH_2 );
			BUBBLE_CATH_PORT |= ( 1 << CATH_3 );
			sendDigitToBubble( bubbleOutput / 1000, DP0 );
			break;
			
		case 1:	// hundreds	
			BUBBLE_CATH_PORT |= ( 1 << CATH_0 );	// turn off the previous cathode 
			BUBBLE_CATH_PORT &= ~( 1 << CATH_1 );	// turn on the current cathode	
			//BUBBLE_CATH_PORT |= ( 1 << CATH_2 );	// no need to keep turning off cathodes that are already off
			//BUBBLE_CATH_PORT |= ( 1 << CATH_3 );	
			sendDigitToBubble( ( bubbleOutput % 1000 ) / 100, DP1 );
			break;
			
		case 2: // tens	
			//BUBBLE_CATH_PORT |= ( 1 << CATH_0 );	
			BUBBLE_CATH_PORT |= ( 1 << CATH_1 );		
			BUBBLE_CATH_PORT &= ~( 1 << CATH_2 );
			//BUBBLE_CATH_PORT |= ( 1 << CATH_3 );
			sendDigitToBubble( ( bubbleOutput % 100 ) / 10, DP2 );
			break;
		
		case 3:	// ones
			//BUBBLE_CATH_PORT |= ( 1 << CATH_0 );	
			//BUBBLE_CATH_PORT |= ( 1 << CATH_1 );		
			BUBBLE_CATH_PORT |= ( 1 << CATH_2 );
			BUBBLE_CATH_PORT &= ~( 1 << CATH_3 );
			sendDigitToBubble( bubbleOutput % 10, DP3 );
			break;
		
		default:
			break;
	}

	if( myCathode < 3 )
	{
		myCathode++;
	}else{
		myCathode = 0;
	}
	#endif
}





#ifdef SERIAL_ENABLE
//unsigned char serialRead(void)
//{
//	while (serialCheckRxComplete() == 0)		// While data is NOT available to read
//	{;;} 
//	return UDR;
//}


//unsigned char serialCheckRxComplete(void)
//{
//	return( UCSRA & _BV(RXC)) ;		// nonzero if serial data is available to read.
//}


void serialWrite(unsigned char DataOut)
{
	while (serialCheckTxReady() == 0)		// while NOT ready to transmit 
	{;;} 
	UDR = DataOut;
}


unsigned char serialCheckTxReady(void)
{
	return( UCSRA & _BV(UDRE) ) ;		// nonzero if transmit register is ready to receive new data.
}
#endif
//~~~~~~~~~~~~~~~~~~~~~~******************** Functions ********************~~~~~~~~~~~~~~~~~~~~~~~~
