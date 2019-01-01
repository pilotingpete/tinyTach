// function prototypes

void uartInit(void);
void toggle_led(void);
void uartPrint( uint32_t data, uint8_t newline );
void timerInit(void);

//unsigned char serialRead(void);
//unsigned char serialCheckRxComplete(void);
void serialWrite(unsigned char DataOut);
unsigned char serialCheckTxReady(void);
uint16_t filter( uint16_t myCPM );


