/* data types  */

#include "ascii.h"



/* ascii temp vars */

uint8_t hexa[4];


/* functions code */




uint8_t AsciiToByte(uint8_t a, uint8_t b)
{
	if ( a >= 65 )
	{
		a = a - 'A' + 10;
	}
	else
	{
		a = a - '0';
	}
	if ( b >= 65 )
	{
		b = b - 'A' + 10;
	}
	else
	{
		b = b - '0';
	}
	return (16 * a) + b;
}

uint8_t NibbleToAscii(uint8_t val)
{     
	if ( val <= 9 )
	{
		return val + '0';
	}
	else
	{
		return (val - 10) + 'A';
	}
}

//void ByteToAscii(uint8_t val,uint8_t* buffer)
//{
//	uint8_t a = val / 16;
//	uint8_t b = val % 16;
//	buffer[0]= NibbleToAscii(a);
//	buffer[1] =NibbleToAscii(b);
//	return ;
//}
//
void ByteToAscii(uint16_t val,uint8_t* buffer,uint8_t noOfBytes)
{

	uint8_t i = 1;

	while ( i < noOfBytes*2)
	{
		buffer[noOfBytes*2-i++]= NibbleToAscii((val % 16));
		val /= 16;
		buffer[noOfBytes*2-i++] =NibbleToAscii((val % 16));
		val /= 16;
	}

	return ;
}

uint16_t AsciiToTwoByte(uint8_t *ascii)
{
	uint16_t a = AsciiToByte(ascii[0], ascii[1]);
	uint16_t b = AsciiToByte(ascii[2], ascii[3]);
	uint16_t result = ( a * 256 ) + b;
	return result;
}
