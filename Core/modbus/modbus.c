#include "modbus.h"

//refer to https://www.modbustools.com/modbus.html

/*************************defines********************************/
#define CR 0x0D
#define LF 0x0A
/*************************typedefs********************************/
typedef struct
{
	uint16_t start;
	uint16_t quantity;
	uint16_t limit;
	uint8_t code;
} Modbus_t;

typedef uint8_t (*ModbusFuncCode_t)(uint8_t *buffer, uint8_t size, Modbus_t mb);
/*************************declares********************************/

uint8_t mbBuffer[ASCII_FRAME_SIZE];

//define buffer for sending data
//TODO:vhange buffer size for RTU and ASCII format
uint8_t sendBuffer[ASCII_FRAME_SIZE];
//buffer for saving Converted ASCII frame to binary frame
uint8_t numericBuffer[ASCII_FRAME_SIZE / 2];
//TODO:implement a callback method for broadcasting and a procedure to register the callback inside
volatile uint8_t broadcast = 0;//flag shows master is broadcasting a request not used here but reserved for future use

union
{
    uint16_t u16_holding_registers_array[MAX_HOLDING_REGISTERS];
    int16_t s16_holding_registers_array[MAX_HOLDING_REGISTERS];
    uint32_t u32_holding_registers_array[MAX_HOLDING_REGISTERS / 2];
    int32_t s32_holding_registers_array[MAX_HOLDING_REGISTERS / 2];
    float f32_holding_registers_array[MAX_HOLDING_REGISTERS / 2];

} holdingRegister;

union
{
    uint16_t u16_input_registers_array[MAX_INPUTS_REGISTERS];
    int16_t s16_input_registers_array[MAX_INPUTS_REGISTERS];
    uint32_t u32_input_registers_array[MAX_INPUTS_REGISTERS / 2];
    int32_t s32_input_registers_array[MAX_INPUTS_REGISTERS / 2];
    float f32_input_registers_array[MAX_INPUTS_REGISTERS / 2];
} inputRegister;

uint8_t coils_array[MAX_COILS];
uint8_t inputs_array[MAX_INPUTS];


//variables to map control code handler into function code. This is some sort of polymorphism :)
struct
{
	ModbusFuncCode_t mbFunc;
} mbState[17];
/*************************static Function prototype********************************/

//general static function
static Modbus_t getModbusInfo(uint8_t *buffer, uint8_t size);
static uint8_t convert2Protocol(uint8_t *byteArgIn, uint8_t size, uint8_t *asciiArgOut);
static uint8_t convert2Byte(uint8_t *asciiArgIn, uint8_t size, uint8_t *byteArgOut);
static uint8_t checkAddressCorrect(uint8_t *buffer, uint8_t size);


//ASCII static function
#ifdef ASCII
static uint8_t checkLRCCorrect(uint8_t *buffer, uint8_t size);
static uint8_t calcLRC(uint8_t *buffer, uint8_t size);
static void generateLRC(uint8_t *buffer, uint8_t size);
#endif

#ifdef RTU
//RTU static function
static uint8_t checkCRCCorrect(uint8_t *buffer, uint8_t size);
static void generateCRC(uint8_t *buffer, uint8_t size);
static uint16_t calcCRC16(const uint8_t *nData, uint16_t wLength);
#endif

//control code static function
static uint8_t ResponseReadCoilsStatus_01(uint8_t *buffer, uint8_t size, Modbus_t mb); // 0x01, OK
static uint8_t ResponseReadInputStatus_02(uint8_t *buffer, uint8_t size, Modbus_t mb); // 0x02, OK!!!
static uint8_t ResponseReadHoldingRegisters_03(uint8_t *buffer, uint8_t size, Modbus_t mb); // 0x03, OK!!!
static uint8_t ResponseReadInputRegisters_04(uint8_t *buffer, uint8_t size, Modbus_t mb); // 0x04,  OK!!!!
static uint8_t ResponseForceSingleCoil_05(uint8_t *buffer, uint8_t size, Modbus_t mb); // 0x05, OK!!!!
static uint8_t ResponsePresetSingleRegister_06(uint8_t *buffer, uint8_t size, Modbus_t mb); //0x06, OK!!!!
static uint8_t ResponseForceMultipleCoils_15(uint8_t *buffer, uint8_t size, Modbus_t mb); // 0x0F, OK
static uint8_t ResponsePresetMultipleRegisters_16(uint8_t *buffer, uint8_t size, Modbus_t mb);

static uint8_t AsciiToByte(uint8_t msb, uint8_t lsb);
static uint8_t NibbleToAscii(uint8_t val);
static void ByteToAscii(uint8_t val,uint8_t* buffer);


static uint16_t GetHoldingRegisterValue_u16_driver(uint16_t adr);
static void SetHoldingRegisterValue_u16_driver(uint16_t adr, uint16_t value);
static uint16_t GetInputRegisterValue_u16_driver(uint16_t adr);

/* holding registers read-write operations */
/*************************implementation********************************/

/***********************************************************
 * Function name: GetHoldingRegisterValue_u16_driver(uint16_t adr)
 * Description: get data from address requested by master in holding register
 * Parameter adr :address requested by master
 * Return value: value in the address
 * Remarks:never use it to get data in your application instead use GetHoldingRegisterValue_u16 function
 ***********************************************************/
static uint16_t GetHoldingRegisterValue_u16_driver(uint16_t adr)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return holdingRegister.u16_holding_registers_array[adr];
    }
}


/***********************************************************
 * Function name:SetHoldingRegisterValue_u16_driver(uint16_t adr, uint16_t value)
 * Description: set data to address requested by master
 * Parameter adr :address requested by master
 * Parameter value :new value to be set in address
 * Return value: None
 * Remarks:never use it to set data in your application instead use SetHoldingRegisterValue_u16 function
 ***********************************************************/
static void SetHoldingRegisterValue_u16_driver(uint16_t adr, uint16_t value)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS || value < 0 || value > 65535)
    {
        return; // invalid register address
    }
    else
    {
        holdingRegister.u16_holding_registers_array[adr] = value;
    }
}

/***********************************************************
 * Function name: GetInputRegisterValue_u16_driver(uint16_t adr)
 * Description: get data from address requested by master in input register
 * Parameter adr :address requested by master
 * Return value: value in the address
 * Remarks:never use it to get data in your application instead use GetInputRegisterValue_u16 function
 ***********************************************************/
static uint16_t GetInputRegisterValue_u16_driver(uint16_t adr)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return inputRegister.u16_input_registers_array[adr];
    }
}


/***********************************************************
 * Function name: AsciiToByte(uint8_t a, uint8_t b)
 * Description: convert two nibble of ASCII hex to a binary byte
 * Parameter msb : most significant nibble of byte
 * Parameter lsb : least significant nibble of byte
 * Return value: converted value in byte
 * Remarks:None
 ***********************************************************/
static uint8_t AsciiToByte(uint8_t msb, uint8_t lsb)
{
	if (msb >= 65)
	{
		msb = msb - 'A' + 10;
	}
	else
	{
		msb = msb - '0';
	}
	if (lsb >= 65)
	{
		lsb = lsb - 'A' + 10;
	}
	else
	{
		lsb = lsb - '0';
	}
	return (16 * msb) + lsb;
}

/***********************************************************
 * Function name: NibbleToAscii(uint8_t val)
 * Description: converts a nibble of binary byte to an ASCII hex
 * Parameter val : nibble of binary must be between 0 and 15
 * Return value: converted value in ascii
 * Remarks:None
 ***********************************************************/
static uint8_t NibbleToAscii(uint8_t val)
{
	if (val <= 9)
	{
		return val + '0';
	}
	else
	{
		return (val - 10) + 'A';
	}
}

/***********************************************************
 * Function name: ByteToAscii(uint8_t val,uint8_t* buffer)
 * Description: converts a nibble of binary byte to an ASCII hex
 * Parameter val : nibble of binary must be between 0 and 15
 * Return value: converted value in ascii
 * Remarks:None
 ***********************************************************/
static void ByteToAscii(uint8_t val,uint8_t* buffer)
{
	uint8_t a = val / 16;
	uint8_t b = val % 16;
	buffer[0]= NibbleToAscii(a);
	buffer[1] =NibbleToAscii(b);
	return ;
}



/***********************************************************
 * Function name: convert2ASCII(uint8_t *byteArgIn, uint8_t size,uint8_t asciiArgOut)
 * Description: convert a buffer of uint8_t stream into a buffer of ASCII code, byte and ascii must not be NULL pointer
 * Parameter byteArgIn : A pointer stream of bytes as an input
 * Parameter size : size of byteArgIn that needs to be converted
 * Parameter asciiArgOut : A pointer to stream of ASCII bytes that will be used to save converted data
 * Return value: size of asciiArgout buffer
 * Remarks:None
 ***********************************************************/
static uint8_t convert2Protocol(uint8_t *byteArgIn, uint8_t size, uint8_t *asciiArgOut)
{
	uint8_t i = 0;

#ifdef ASCII
	generateLRC(byteArgIn, size);
	asciiArgOut[0] = ':';
	for (i = 0; i < size + 1; i++)
	{
		ByteToAscii(byteArgIn[i], &sendBuffer[2 * i + 1]);
	}
	asciiArgOut[2 * size + 3] = CR;
	asciiArgOut[2 * size + 4] = LF;
	return 2 * size + 5;
#else
	generateCRC(byteArgIn, size);
	for (i = 0; i < size + 2; i++)
	{
		sendBuffer[i] = byteArgIn[i];
	}
	return size + 2;
#endif
}

/***********************************************************
 * Function name: convert2Byte(uint8_t *asciiArgIn, uint8_t size,uint8_t *byteArgOut)
 * Description: convert a buffer of ASCII code into a buffer of uint8_t stream, byte and ascii must not be NULL pointer
 * Parameter asciiArgIn : A pointer to stream of ASCII bytes as an input
 * Parameter size : size of asciiArgIn that needs to be converted
 * Parameter byteArgOut : A pointer to stream of bytes that will be used to save converted data
 * Return value: 1 if success 0 if failed
 * Remarks:byteArgOut must be large enough so that fucntion not write out of allocated area
 ***********************************************************/
static uint8_t convert2Byte(uint8_t *asciiArgIn, uint8_t size, uint8_t *byteArgOut)
{
	if ((byteArgOut == NULL) || (asciiArgIn == NULL))
	{
		return 0;
	}
#ifdef ASCII
	uint8_t i = 0;
	for (i = 1; i < size; i += 2)
	{
		byteArgOut[i / 2] = AsciiToByte(asciiArgIn[i], asciiArgIn[i + 1]);
	}

#endif

	return 1;
}

/***********************************************************
 * Function name: getModbusInfo(uint8_t *buffer, uint8_t size)
 * Description: get the info such as start, quantity from a received packet
 * Parameter buffer : A pointer to stream of received buffer as an input
 * Parameter size : size of buffer sent to function
 * Return value: a Modbus_t variable that contains extracted info
 * Remarks:None
 ***********************************************************/
static Modbus_t getModbusInfo(uint8_t *buffer, uint8_t size)
{
	Modbus_t mb = {0};

	mb.start = buffer[2] * 256 + buffer[3];
	mb.quantity = buffer[4] * 256 + buffer[5];
	mb.limit = mb.start + mb.quantity;
	mb.code = buffer[1];
	return mb;
}

/***********************************************************
 * Function name: checkAddressCorrect(uint8_t *buffer, uint8_t size)
 * Description: checks if received address is the same as device address
 * Parameter buffer : a pointer to received buffer
 * Parameter size : size of received buffer
 * Return value: return 1 if matches else returns 0
 * Remarks:None
 ***********************************************************/
static uint8_t checkAddressCorrect(uint8_t *buffer, uint8_t size)
{
	uint8_t is_me = 0;
	/*   frame ok  */
	is_me = buffer[0];
	if (is_me == 0)
	{
		broadcast = 1;
		return 1;
	}

	broadcast = 0;
	if (DEVICE_ID != is_me)
	{
		return 0;
	}

	return 1;
}





#ifdef ASCII
/***********************************************************
 * Function name: checkLRCCorrect(uint8_t *buffer, uint8_t size)
 * Description: check if input buffer has correct LRC or not
 * Parameter buffer : a pointer to buffer that is going to be checked
 * Parameter size : size of buffer to be checked
 * Return value: returns 1 if success 0 if failed
 * Remarks:None
 ***********************************************************/
static uint8_t checkLRCCorrect(uint8_t *buffer, uint8_t size)
{


	if (calcLRC(buffer, size - 2) != buffer[size - 2])
	{
		//clear_frame(buffer); // bad LRC, frame discard
		return 0;
	}
	return 1;
}
/***********************************************************
 * Function name: calcLRC(uint8_t *buffer, uint8_t size)
 * Description: calculate input buffer LRC
 * Parameter buffer : a pointer to buffer that is going to be calculated
 * Parameter size : size of buffer to be calculated
 * Return value: returns LRC
 * Remarks:None
 ***********************************************************/
static uint8_t calcLRC(uint8_t *buffer, uint8_t size)
{
	uint8_t result = 0, i = 0;
	for (i = 0; i < size; i++)
	{
		result += buffer[i];
	}
	result = ~result;
	return (result + 1);
}
/***********************************************************
 * Function name: calcLRC(uint8_t *buffer, uint8_t size)
 * Description: insert LRC in the end of buffer
 * Parameter buffer : a pointer to buffer that is going to be inserted
 * Parameter size : size of buffer that is going to be inserted
 * Return value: None
 * Remarks:None
 ***********************************************************/
static void generateLRC(uint8_t *buffer, uint8_t size)
{
	buffer[size] = calcLRC(buffer, size);
}

#endif


#ifdef RTU
/***********************************************************
 * Function name: calcCRC16(const uint8_t *nData, uint16_t wLength)
 * Description: Calculate CRC for RTU modbus input buffer
 * Parameter nData : a pointer to buffer that is going to be calculated
 * Parameter wLength : length of buffer that is going to be calculated
 * Return value: calculated CRC
 * Remarks:None
 ***********************************************************/
static uint16_t calcCRC16(const uint8_t *nData, uint16_t wLength)
{
	static const uint16_t wCRCTable[] = {
		0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
		0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
		0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
		0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
		0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
		0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
		0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
		0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
		0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
		0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
		0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
		0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
		0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
		0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
		0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
		0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
		0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
		0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
		0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
		0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
		0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
		0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
		0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
		0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
		0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
		0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
		0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
		0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
		0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
		0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
		0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
		0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040};

	uint8_t nTemp;
	uint16_t wCRCWord = 0xFFFF;

	while (wLength--)
	{
		nTemp = *nData++ ^ wCRCWord;
		wCRCWord >>= 8;
		wCRCWord ^= wCRCTable[nTemp];
	}
	return wCRCWord;
} // End: calcCRC16
/***********************************************************
 * Function name: generateCRC(uint8_t *buffer, uint8_t size)
 * Description: insert CRC and LR CF in the end of buffer
 * Parameter buffer : a pointer to buffer that is going to be inserted
 * Parameter size : size of buffer that is going to be inserted
 * Return value: None
 * Remarks:None
 ***********************************************************/
static void generateCRC(uint8_t *buffer, uint8_t size)
{
	union
	{
		uint16_t val;
		uint8_t buff[2];
	} data;

	data.val = calcCRC16(buffer, size);

	buffer[size] = data.buff[0];
	buffer[size + 1] = data.buff[1];
}

/***********************************************************
 * Function name: checkCRCCorrect(uint8_t *buffer, uint8_t size)
 * Description: check if input buffer has correct CRC or not
 * Parameter buffer : a pointer to buffer that is going to be checked
 * Parameter size : size of buffer to be checked
 * Return value: returns 1 if success 0 if failed
 * Remarks:None
 ***********************************************************/
static uint8_t checkCRCCorrect(uint8_t *buffer, uint8_t size)
{
	union
	{
		uint16_t val;
		uint8_t buff[2];
	} data;
	uint16_t crc = calcCRC16(buffer, size - 2);
	data.buff[0] = buffer[size - 2];
	data.buff[1] = buffer[size - 1];

	if (data.val != crc)
	{
		return 0;
	}
	return 1;
}

#endif


/***********************************************************
 * Function name: ResponseReadCoilsStatus_01(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 1 of Modbus of reading coil
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponseReadCoilsStatus_01(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x01, OK
{

	uint8_t newDataCount = 0, i, k, inputs; //update index to add data

	/* querying inputs status */
	for (i = mb.start; i < mb.limit; i += 8)
	{
		k = 0;
		inputs = 0;
		while (k < 8 && i + k < mb.limit)
		{
			uint8_t val = GetCoilValue(i + k); // status of input i+k
			if (val == 1)					   //input active
			{
				inputs |= (1 << k);
			}
			k++;
		}
		buffer[newDataCount + 3] = inputs; //
		newDataCount++;
	}
	buffer[2] = newDataCount; //

	//ByteToAscii(newDataCount, &sendBuffer[5], 1);
	return 3 + newDataCount;
}
/***********************************************************
 * Function name: ResponseReadInputStatus_02(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 2 of Modbus of reading input
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponseReadInputStatus_02(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x02, OK!!!
{
	uint8_t newDataCount = 0, i, k, inputs; //update index to add data

	/* querying inputs status */
	for (i = mb.start; i < mb.limit; i += 8)
	{
		k = 0;
		inputs = 0;
		while (k < 8 && i + k < mb.limit)
		{
			uint8_t val = GetInputStatusValue(i + k); // status of input i+k
			if (val == 1)							  //input active
			{
				inputs |= (1 << k);
			}
			k++;
		}
		numericBuffer[newDataCount + 3] = inputs; //
		newDataCount++;
	}
	numericBuffer[2] = newDataCount; //
	return 3 + newDataCount;
}
/***********************************************************
 * Function name: ResponseReadHoldingRegisters_03(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 3 of Modbus of reading holding registers
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponseReadHoldingRegisters_03(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x03, OK!!!
{
	union
	{
		uint16_t val;
		uint8_t buff[2];
	} data;
	uint8_t dataCount = 0;
	// ByteToAscii(, &sendBuffer[5], 1);
	buffer[2] = (mb.limit - mb.start) * 2;
	/* querying holding registers status */
	for (int i = mb.start; i < mb.limit; i++)
	{
		//implement here what is needed to get holding registers
		data.val = GetHoldingRegisterValue_u16_driver(i); //put counter here for test
		buffer[3 + dataCount * 2] = data.buff[1];
		buffer[3 + dataCount * 2 + 1] = data.buff[0];
		dataCount++;
	}
	buffer[2] = dataCount * 2;
	return dataCount * 2 + 3;
}
/***********************************************************
 * Function name: ResponseReadInputRegisters_04(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 4 of Modbus of reading input registers
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponseReadInputRegisters_04(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x04,  OK!!!!
{

	union
	{
		uint16_t val;
		uint8_t buff[2];
	} data;
	uint8_t dataCount = 0;
	// ByteToAscii(, &sendBuffer[5], 1);
	buffer[2] = (mb.limit - mb.start) * 2;
	/* querying holding registers status */
	for (int i = mb.start; i < mb.limit; i++)
	{
		//implement here what is needed to get holding registers
		data.val = GetInputRegisterValue_u16_driver(i); //put counter here for test
		buffer[3 + dataCount * 2] = data.buff[1];
		buffer[3 + dataCount * 2 + 1] = data.buff[0];
		dataCount++;
	}
	buffer[2] = dataCount * 2;
	return dataCount * 2 + 3;
}
/***********************************************************
 * Function name: ResponseForceSingleCoil_05(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 5 of Modbus of writing single coil
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponseForceSingleCoil_05(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x05, OK!!!!
{
	uint16_t coilID = 0;
	uint16_t value = 0;

	coilID = mb.start; //coil index

	value = mb.quantity;			  //coil value
	if (value == 0 || value == 65280) // 0 or 1, 0000h or FF00h
	{
		SetCoilValue(coilID, value > 0);
	}

	return 6;
}

/***********************************************************
 * Function name: ResponsePresetSingleRegister_06(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 6 of Modbus of writing single registers
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponsePresetSingleRegister_06(uint8_t *buffer, uint8_t size, Modbus_t mb) //0x06, OK!!!!
{
	uint16_t registerID = 0;
	uint16_t value = 0;

	registerID = mb.start; //register index

	value = mb.quantity; //register value
	SetHoldingRegisterValue_u16_driver(registerID, value);

	return 6;
}

/***********************************************************
 * Function name: ResponseForceMultipleCoils_15(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 15 of Modbus of writing multiple coil
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponseForceMultipleCoils_15(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x0F, OK
{

	uint16_t i = 0;
	uint8_t coils = 0;
	uint8_t k = 0;
	uint8_t new_data_count = 0;
	uint8_t tempAdr = 0;
	uint8_t index = 0, offset = 0;
	uint16_t parameters[20];
	uint8_t byte_count;
	uint8_t data_count = 7;
	//
	new_data_count = buffer[6];

	//
	//    /* read new coils values */
	byte_count = 0;
	for (i = 0; i < new_data_count; i++)
	{
		coils = buffer[data_count];
		data_count += 1;
		parameters[byte_count++] = coils;
	}
	//
	//    /* force coils status */
	for (i = mb.start; i < mb.limit; i++)
	{
		tempAdr = i - mb.start;
		index = tempAdr / 8;
		offset = tempAdr % 8;
		k = (parameters[index] & (1 << offset));
		SetCoilValue(i, k > 0); // force status of coil i with k
	}

	return 6;
}

/***********************************************************
 * Function name: ResponsePresetMultipleRegisters_16(uint8_t *buffer, uint8_t size, Modbus_t mb)
 * Description: process control code 16 of Modbus of writing multiple registers
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
static uint8_t ResponsePresetMultipleRegisters_16(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x10, OK!!!
{

	uint16_t i = 0,  sendIndex = 0;
	uint8_t byte_count;
	union
	{
		uint16_t val;
		uint8_t buff[2];
	} data;
	byte_count = buffer[6];
	sendIndex = 7;

	/* read and set new holding registers values */

	for (i = 0; i < byte_count / 2; i++)
	{
		data.buff[1] = buffer[sendIndex++];
		data.buff[0] = buffer[sendIndex++];
		SetHoldingRegisterValue_u16_driver(mb.start++, data.val);
	}

	return 6;
}


/***********************************************************
 * Function name: initModbus(void)
 * Description: initialize modbus in order to run correctly, It must be called once to activate protocol
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: point to the end position of modified buffer
 * Remarks:None
 ***********************************************************/
void initModbus(void)
{
	//register response function pointer to modbus state array in order to map function code to appropriate function
	mbState[1].mbFunc = ResponseReadCoilsStatus_01;
	mbState[2].mbFunc = ResponseReadInputStatus_02;
	mbState[3].mbFunc = ResponseReadHoldingRegisters_03;
	mbState[4].mbFunc = ResponseReadInputRegisters_04;
	mbState[5].mbFunc = ResponseForceSingleCoil_05;
	mbState[6].mbFunc = ResponsePresetSingleRegister_06;
	mbState[15].mbFunc = ResponseForceMultipleCoils_15;
	mbState[16].mbFunc = ResponsePresetMultipleRegisters_16;

}

/***********************************************************
 * Function name: execute_modbus_command(uint8_t *buffer, uint8_t size)
 * Description: This function process received buffer from master and send data to the output
 * Parameter buffer : a pointer to buffer that is going to be processed
 * Parameter size : size of buffer to be processed
 * parameter mb : Modbus_t type to send received packet information
 * Return value: size of response
 * Remarks:None
 ***********************************************************/
uint8_t execute_modbus_command(uint8_t *buffer, uint8_t size)
{
	uint8_t sendIndex;
	Modbus_t mb;
	uint8_t numericSize;
	uint8_t *selectBuff;

#ifdef ASCII
	selectBuff = numericBuffer;
	if (!convert2Byte(buffer, size, selectBuff))
	{
		//input array is NULL. please allocate memory for input
		while (1);
	}

	numericSize = (size - 1) / 2;
	if (!checkLRCCorrect(selectBuff, numericSize))
	{
		//not LRC correct
		return 0;
	}

	if (size < 9)
	{
		//clear_frame(buffer); // bad frame discard , minimun 9 characters for valid
		return 0;
	}

#else
	selectBuff = buffer;
	numericSize = size;
	if (!checkCRCCorrect(selectBuff, numericSize))
	{
		return 0;
	}
#endif

	/*   frame ok  */
	if (!checkAddressCorrect(selectBuff, numericSize))
	{
		//Not this device address
		return 0;
	}

	mb = getModbusInfo(selectBuff, numericSize);

	//call function code]
	if (mbState[mb.code].mbFunc != NULL)
		sendIndex = mbState[mb.code].mbFunc(selectBuff, numericSize, mb);

	sendIndex = convert2Protocol(selectBuff, sendIndex, sendBuffer);
	if (broadcast == 1)
		{
			broadcast = 0;

			return 0;
		}


	//transfer send buffer asynchronously by DMA

	return sendIndex;
}

/***********************************************************
 * Function name:GetCoilValue(uint16_t adr)
 * Description: get coil data from address passed by user
 * Parameter adr :address requested by user
 * Return value: a byte in which the corresponding bit related to address is 1 if ON otherwise in 0
 * Remarks:None
 ***********************************************************/
uint8_t GetCoilValue(uint16_t adr)
{
    uint8_t res = 0, i = 0, d = 0;
    if (adr < 0 || adr > MAX_COILS * 8)
    {
        return 0;
    }
    i = adr / 8;
    d = adr % 8;
    res = (coils_array[i] & (1 << d));
    return res > 0;
}
/***********************************************************
 * Function name: SetCoilValue(uint16_t adr, uint8_t value)
 * Description: turn on or off coil bit corresponds to address passed by user
 * Parameter adr :address requested by user
 * Parameter value : uint8_t type value to be set either is 1 or 0
 * Return value: None
 * Remarks:None
 ***********************************************************/
void SetCoilValue(uint16_t adr, uint8_t value)
{
    uint8_t res = 0, i = 0, d = 0;
    if (adr < 0 || adr > MAX_COILS * 8)
    {
        return;
    }
    i = adr / 8;
    d = adr % 8;
    res = (coils_array[i] & (1 << d));
    if (res > 0)
    {
        res = 1; //coil on
    }
    else
    {
        res = 0; //coil off
    }
    if (res == 1 && value == 1)
        return; // already ON
    if (res == 0 && value == 0)
        return;                 // already OFF
    if (res == 0 && value == 1) // is off turn on
    {
        coils_array[i] = (coils_array[i] | (1 << d)); // active coil
    }
    if (res == 1 && value == 0) // is on turn off
    {
        coils_array[i] = (coils_array[i] & (~(1 << d))); // desactive coil
    }
    return;
}

void setCoilMulti(uint8_t address,uint8_t value){
	//TODO: implement this to use with code 15
}


/***********************************************************
 * Function name: GetHoldingRegisterValue_u16(uint16_t adr)
 * Description: get 16-bit unsigned integer data from address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: uint16_t value read from address
 * Remarks:None
 ***********************************************************/
uint16_t GetHoldingRegisterValue_u16(uint16_t adr)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return holdingRegister.u16_holding_registers_array[adr - 1];
    }
}

/***********************************************************
 * Function name: GetHoldingRegisterValue_s16(uint16_t adr)
 * Description: get 16-bit signed integer data from address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: int16_t value read from address
 * Remarks:None
 ***********************************************************/
int16_t GetHoldingRegisterValue_s16(uint16_t adr)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return holdingRegister.s16_holding_registers_array[adr - 1];
    }
}

//care must be taken to pass correct address because it returns 32-bit unsigned
/***********************************************************
 * Function name: GetHoldingRegisterValue_u32(uint16_t adr)
 * Description: get 32-bit unsigned integer data from address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: uint32_t value read from address
 * Remarks:None
 ***********************************************************/
uint32_t GetHoldingRegisterValue_u32(uint16_t adr)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {

        return holdingRegister.u32_holding_registers_array[adr];
    }
}
/***********************************************************
 * Function name: GetHoldingRegisterValue_s32(uint16_t adr)
 * Description: get 32-bit signed integer data from address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: int32_t value read from address
 * Remarks:None
 ***********************************************************/
int32_t GetHoldingRegisterValue_s32(uint16_t adr)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return holdingRegister.s32_holding_registers_array[adr];
    }
}
/***********************************************************
 * Function name: GetHoldingRegisterValue_f32(uint16_t adr)
 * Description: get 32-bit float data from address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: float value read from address
 * Remarks:None
 ***********************************************************/
float GetHoldingRegisterValue_f32(uint16_t adr)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return holdingRegister.f32_holding_registers_array[adr];
    }
}
/***********************************************************
 * Function name: SetHoldingRegisterValue_u16(uint16_t adr, uint16_t value)
 * Description: set 16-bit unsigned integer data to address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: uint16_t value read from address
 * Remarks:None
 ***********************************************************/
void SetHoldingRegisterValue_u16(uint16_t adr, uint16_t value)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS || value < 0 || value > 65535)
    {
        return; // invalid register address
    }
    else
    {
        holdingRegister.u16_holding_registers_array[adr - 1] = value;
    }
}
/***********************************************************
 * Function name: SetHoldingRegisterValue_s16(uint16_t adr, int16_t value)
 * Description: set 16-bit signed integer data to address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: int16_t value read from address
 * Remarks:None
 ***********************************************************/
void SetHoldingRegisterValue_s16(uint16_t adr, int16_t value)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return; // invalid register address
    }
    else
    {
        holdingRegister.s16_holding_registers_array[adr - 1] = value;
    }
}
/***********************************************************
 * Function name: SetHoldingRegisterValue_u32(uint16_t adr, uint32_t value)
 * Description: set 32-bit unsigned integer data to address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: uint32_t value read from address
 * Remarks:None
 ***********************************************************/
void SetHoldingRegisterValue_u32(uint16_t adr, uint32_t value)
{

    if (adr < 0 || adr > MAX_HOLDING_REGISTERS || value < 0)
    {
        return; // invalid register address
    }
    else
    {
        holdingRegister.u32_holding_registers_array[adr] = value;
    }
}
/***********************************************************
 * Function name: SetHoldingRegisterValue_s32(uint16_t adr, int32_t value)
 * Description: set 32-bit signed integer data to address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: int32_t value read from address
 * Remarks:None
 ***********************************************************/
void SetHoldingRegisterValue_s32(uint16_t adr, int32_t value)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return; // invalid register address
    }
    else
    {
        holdingRegister.s32_holding_registers_array[adr] = value;
    }
}
/***********************************************************
 * Function name: SetHoldingRegisterValue_f32(uint16_t adr, float value)
 * Description: set 32-bit float data to address passed by user in holding register
 * Parameter adr :address requested by user
 * Return value: float value read from address
 * Remarks:None
 ***********************************************************/
void SetHoldingRegisterValue_f32(uint16_t adr, float value)
{
    if (adr < 0 || adr > MAX_HOLDING_REGISTERS)
    {
        return; // invalid register address
    }
    else
    {
        holdingRegister.f32_holding_registers_array[adr] = value;
    }
}
/***********************************************************
 * Function name: GetInputRegisterValue_u16(uint16_t adr)
 * Description: get 16-bit unsigned integer data from address passed by user in input register
 * Parameter adr :address requested by user
 * Return value: uint16_t value read from address
 * Remarks:None
 ***********************************************************/
uint16_t GetInputRegisterValue_u16(uint16_t adr)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return inputRegister.u16_input_registers_array[adr - 1];
    }
}
/***********************************************************
 * Function name: GetInputRegisterValue_s16(uint16_t adr)
 * Description: get 16-bit signed integer data from address passed by user in input register
 * Parameter adr :address requested by user
 * Return value: int16_t value read from address
 * Remarks:None
 ***********************************************************/
int16_t GetInputRegisterValue_s16(uint16_t adr)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return inputRegister.s16_input_registers_array[adr];
    }
}

//care must be taken to pass correct address because it returns 32-bit unsigned
/***********************************************************
 * Function name: GetInputRegisterValue_u32(uint16_t adr)
 * Description: get 32-bit unsigned integer data from address passed by user in input register
 * Parameter adr :address requested by user
 * Return value: uint32_t value read from address
 * Remarks:None
 ***********************************************************/
uint32_t GetInputRegisterValue_u32(uint16_t adr)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return inputRegister.u32_input_registers_array[adr];
    }
}
/***********************************************************
 * Function name: GetInputRegisterValue_s32(uint16_t adr)
 * Description: get 32-bit signed integer data from address passed by user in input register
 * Parameter adr :address requested by user
 * Return value: int32_t value read from address
 * Remarks:None
 ***********************************************************/
int32_t GetInputRegisterValue_s32(uint16_t adr)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return inputRegister.s32_input_registers_array[adr];
    }
}
/***********************************************************
 * Function name: GetInputRegisterValue_f32(uint16_t adr)
 * Description: get 32-bit float data from address passed by user in input register
 * Parameter adr :address requested by user
 * Return value: float value read from address
 * Remarks:None
 ***********************************************************/
float GetInputRegisterValue_f32(uint16_t adr)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return 0; // invalid register address
    }
    else
    {
        return inputRegister.f32_input_registers_array[adr];
    }
}


/***********************************************************
 * Function name: SetInputStatusValue(uint16_t adr, uint8_t value)
 * Description: turn on or off input status bit corresponds to address passed by user
 * Parameter adr :address requested by user
 * Parameter value : uint8_t type value to be set either is 1 or 0
 * Return value: None
 * Remarks:None
 ***********************************************************/
void SetInputStatusValue(uint16_t adr, uint8_t value)
{
    uint8_t res = 0, i = 0, d = 0;
    if (adr < 0 || adr > MAX_COILS * 8)
    {
        return;
    }
    i = adr / 8;
    d = adr % 8;
    res = (inputs_array[i] & (1 << d));
    if (res > 0)
    {
        res = 1; //coil on
    }
    else
    {
        res = 0; //coil off
    }
    if (res == 1 && value == 1)
        return; // already ON
    if (res == 0 && value == 0)
        return;                 // already OFF
    if (res == 0 && value == 1) // is off turn on
    {
    	inputs_array[i] = (inputs_array[i] | (1 << d)); // active coil
    }
    if (res == 1 && value == 0) // is on turn off
    {
    	inputs_array[i] = (inputs_array[i] & (~(1 << d))); // desactive coil
    }
    return;
}
/***********************************************************
 * Function name: GetInputStatusValue(uint16_t adr)
 * Description: get input status data from address passed by user
 * Parameter adr :address requested by user
 * Return value: a byte in which the corresponding bit related to address is 1 if ON otherwise in 0
 * Remarks:None
 ***********************************************************/
uint8_t GetInputStatusValue(uint16_t adr)
{
    uint8_t res = 0, i = 0, d = 0;
    if (adr < 0 || adr > MAX_INPUTS * 8)
    {
        return 0;
    }
    i = adr / 8;
    d = adr % 8;
    res = (inputs_array[i] & (1 << d));
    return res > 0;
}
/***********************************************************
 * Function name: SetInputRegisterValue_u16(uint16_t adr, uint16_t value)
 * Description: set 16-bit unsigned integer data to address passed by user in input register
 * Parameter adr :address requested by user
 * Parameter value : uint16_t type value to be set
 * Return value: None
 * Remarks:None
 ***********************************************************/
void SetInputRegisterValue_u16(uint16_t adr, uint16_t value)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS || value < 0 || value > 65535)
    {
        return; // invalid register address
    }
    else
    {
        inputRegister.u16_input_registers_array[adr - 1] = value;
    }
}
/***********************************************************
 * Function name: SetInputRegisterValue_s16(uint16_t adr, int16_t value)
 * Description: set 16-bit signed integer data to address passed by user in input register
 * Parameter adr :address requested by user
 * Parameter value : int16_t type value to be set
 * Return value: None
 * Remarks:None
 ***********************************************************/
void SetInputRegisterValue_s16(uint16_t adr, int16_t value)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return; // invalid register address
    }
    else
    {
        inputRegister.s16_input_registers_array[adr - 1] = value;
    }
}
/***********************************************************
 * Function name: SetInputRegisterValue_u32(uint16_t adr, uint32_t value)
 * Description: set 32-bit unsigned integer data to address passed by user in input register
 * Parameter adr :address requested by user
 * Parameter value : uint32_t type value to be set
 * Return value: None
 * Remarks:None
 ***********************************************************/
void SetInputRegisterValue_u32(uint16_t adr, uint32_t value)
{

    if (adr < 0 || adr > MAX_INPUTS_REGISTERS || value < 0)
    {
        return; // invalid register address
    }
    else
    {
        inputRegister.u32_input_registers_array[adr] = value;
    }
}
/***********************************************************
 * Function name: SetInputRegisterValue_s32(uint16_t adr, int32_t value)
 * Description: set 32-bit signed integer data to address passed by user in input register
 * Parameter adr :address requested by user
 * Parameter value : int32_t type value to be set
 * Return value: None
 * Remarks:None
 ***********************************************************/
void SetInputRegisterValue_s32(uint16_t adr, int32_t value)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return; // invalid register address
    }
    else
    {
        inputRegister.s32_input_registers_array[adr] = value;
    }
}

/***********************************************************
 * Function name: SetInputRegisterValue_f32(uint16_t adr, float value)
 * Description: set 32-bit float data to address passed by user in input register
 * Parameter adr :address requested by user
 * Parameter value : 32-bit float value to be set
 * Return value: None
 * Remarks:None
 ***********************************************************/
void SetInputRegisterValue_f32(uint16_t adr, float value)
{
    if (adr < 0 || adr > MAX_INPUTS_REGISTERS)
    {
        return; // invalid register address
    }
    else
    {
        inputRegister.f32_input_registers_array[adr] = value;
    }
}
