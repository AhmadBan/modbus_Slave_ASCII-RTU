/* functions prototypes */
#include "main.h"
extern uint8_t ascii[4];

uint8_t NibbleToAscii(uint8_t);
uint8_t AsciiToByte(uint8_t, uint8_t);

uint16_t AsciiToTwoByte(uint8_t *ascii);

/* functions code */

void ByteToAscii(uint8_t val,uint8_t* buffer);
uint8_t NibbleToAscii(uint8_t val);

uint8_t AsciiToByte(uint8_t a, uint8_t b);

void twoByteToAscii(uint16_t val, uint8_t *buffer, uint8_t noOfBytes);
//void TwoByteToAscii(uint16_t val,uint8_t* buffer);
uint16_t AsciiToTwoByte();
