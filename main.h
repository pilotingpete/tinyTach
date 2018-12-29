// function prototypes

void ioInit(void);
void uartInit(void);
void toggle_led(void);
void uartPrint( uint32_t data, uint8_t newline );
void timerInit(void);
void sendDigitToBubble( uint8_t digitToSend, uint8_t doDecimal );
void updateBubbleDisplay( void );
void shiftRegisterPulse( void );
void shiftByteOut( uint8_t byteToShift );
//unsigned char serialRead(void);
//unsigned char serialCheckRxComplete(void);
void serialWrite(unsigned char DataOut);
unsigned char serialCheckTxReady(void);
uint16_t filter( uint16_t myCPM );


