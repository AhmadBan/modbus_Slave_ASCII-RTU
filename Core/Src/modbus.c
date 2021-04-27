#include "modbus.h"
extern UART_HandleTypeDef huart2;
/* ASCII FRAME */
uint8_t ascii_frame[ASCII_FRAME_SIZE]; // max size = 255
uint8_t data_count = 0;
uint8_t send_count = 0;
uint8_t broadcast = 0;
uint8_t sendBuffer[ASCII_FRAME_SIZE];
uint8_t numericBuffer[ASCII_FRAME_SIZE/2];
struct
{
    ModbusFuncCode_t mbFunc;
} mbState[17];

/***********************************************************
* Function name: convert2ASCII(uint8_t *byteArgIn, uint8_t size,uint8_t asciiArgOut)
* Description: convert a buffer of uint8_t stream into a buffer of ASCII code, byte and ascii must not be NULL pointer
* Parameter byteArgIn : A pointer stream of bytes as an input
* Parameter size : size of byteArgIn that needs to be converted
* Parameter asciiArgOut : A pointer to stream of ASCII bytes that will be used to save converted data
* Return value: size of asciiArgout buffer
* Remarks:None
***********************************************************/
static uint8_t convert2ASCII(uint8_t *byteArgIn, uint8_t size,uint8_t* asciiArgOut)
{
#ifdef ASCII
	uint8_t i=0;
	gen_lrc(byteArgIn, size);
	asciiArgOut[0]=':';
	for(i=0;i<size+1;i++){
		ByteToAscii(byteArgIn[i],&sendBuffer[2*i+1]);
	}
	asciiArgOut[2*size+3]=CR;
	asciiArgOut[2*size+4]=LF;



#elif RTU


#endif

    return 2*size+5;
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
static uint8_t convert2Byte(uint8_t *asciiArgIn, uint8_t size,uint8_t *byteArgOut)
{
	if((byteArgOut==NULL)||(asciiArgIn==NULL)){
		return 0;
	}
#ifdef ASCII
	uint8_t i=0;
		for(i=1;i<size;i+=2){
			byteArgOut[i/2]=AsciiToByte(asciiArgIn[i], asciiArgIn[i+1]);
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

    mb.start = buffer[2]*256+buffer[3];
    mb.quantity =  buffer[4]*256+buffer[5];
    mb.limit = mb.start + mb.quantity;
    mb.code=buffer[1];
    return mb;
}
static uint8_t checkLRCCorrect(uint8_t *buffer, uint8_t size)
{
    uint8_t lrc = 0;

    if (lrc_calc(buffer, size-2) !=buffer[size-2] )
    {
        //clear_frame(buffer); // bad LRC, frame discard
        return 0;
    }
    return 1;
}

static uint8_t checkCRCCorrect(uint8_t *buffer, uint8_t size)
{

    return 1;
}

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
    if (DIR != is_me)
    {
        return 0;
    }

    return 1;
}

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

uint8_t execute_modbus_command(uint8_t *buffer, uint8_t size)
{
    uint8_t sendIndex;
    Modbus_t mb;
    uint8_t numericSize;

#ifdef ASCII
    if(!convert2Byte(buffer,size,numericBuffer)){
    	//input array is NULL. please allocate memory for input
    	while(1);
    }
    numericSize=(size-1)/2;
    if (!checkLRCCorrect(numericBuffer, numericSize))
       {
           //not LRC correct
           return 0;
       }

    if (size < 9)
    {
        //clear_frame(buffer); // bad frame discard , minimun 9 characters for valid
        return 0;
    }

    #elif RTU

    if (!checkCRCCorrect(numericBuffer, size)){
    	return 0;
    }
#endif






    /*   frame ok  */
    if (!checkAddressCorrect(numericBuffer, numericSize))
    {
        //Not this device address
        return 0;
    }

    mb = getModbusInfo(numericBuffer, numericSize);

    //call function code]
    if(mbState[mb.code].mbFunc!=NULL)
    	sendIndex = mbState[mb.code].mbFunc(numericBuffer, numericSize, mb);

    sendIndex=convert2ASCII(numericBuffer,sendIndex,sendBuffer);


    //transfer send buffer asynchronously by DMA
    tx_ascii_frame(sendBuffer, sendIndex);
    return 1;
}

void gen_lrc(uint8_t *buffer, uint8_t size)
{
    buffer[size]=lrc_calc(buffer, size);
}
void clear_frame(uint8_t *buffer)
{
    uint8_t i = 0;
    data_count = 0;
    send_count = 0;
    for (i = 0; i < ASCII_FRAME_SIZE; i++)
    {
        buffer[i] = 0;
    }
}
void tx_ascii_frame(uint8_t *buffer, uint8_t size)
{
    if (broadcast == 1)
    {
        broadcast = 0;
        clear_frame(buffer);
        return;
    }
    HAL_UART_Transmit_DMA(&huart1, buffer, size);
}
uint8_t lrc_calc(uint8_t *buffer, uint8_t size)
{
    uint8_t result = 0, i = 0;
    for (i = 0; i < size; i++)
    {
        result += buffer[i];
    }
    result = ~result;
    return (result + 1);
}

//refer to https://www.modbustools.com/modbus.html
/*
 * Example Modbus ASCII frame
 * Here is an example of a Modbus ASCII request for the content of analog output holding registers #40108 to #40110 from the slave device with address 21.
 * : 1 5 0 3 0 0 6 B 0 0 0 3 7 A CR LF
 * 1 5: The SlaveID Address (21 = 0x15 = '0' and '5')
 * 0 3: The function code (read analog output holding registers)
 * 0 0 6 B: The data address of the first register requested. (40108 - 40001 offset = 107 = 0x6B)
 * 0 0 0 3: The total number of registers requested. (read 3 registers 40108 to 40110)
 * 7 A: The LRC (Longitudinal redundancy check) for error checking.
 */

/* function codes */
uint8_t ResponseReadCoilsStatus_01(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x01, OK
{

    uint8_t  newDataCount = 0, i, k, inputs; //update index to add data

    /* querying inputs status */
    for (i = mb.start; i < mb.limit; i += 8)
    {
        k = 0;
        inputs = 0;
        while (k < 8 && i + k < mb.limit)
        {
            uint8_t val = GetCoilValue(i + k); // status of input i+k
            if (val == 1)                      //input active
            {
                inputs |= (1 << k);
            }
            k++;
        }
        numericBuffer[newDataCount+3]=inputs;//
        newDataCount++;
    }
    numericBuffer[2]=newDataCount;//

    //ByteToAscii(newDataCount, &sendBuffer[5], 1);
    return 3+newDataCount;
}
uint8_t ResponseReadInputStatus_02(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x02, OK!!!
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
            if (val == 1)                                   //input active
            {
                inputs |= (1 << k);
            }
            k++;
        }
        numericBuffer[newDataCount+3]=inputs;//
        newDataCount++;

    }
    numericBuffer[2]=newDataCount;//
    return 3+newDataCount;
}

uint16_t counter = 1;
uint8_t ResponseReadHoldingRegisters_03(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x03, OK!!!
{
	union{
    uint16_t val ;
    uint8_t buff[2];
	}data;
	uint8_t dataCount=0;
   // ByteToAscii(, &sendBuffer[5], 1);
    buffer[2]=(mb.limit - mb.start) * 2;
    /* querying holding registers status */
    for (int i = mb.start; i < mb.limit; i++)
    {
        //implement here what is needed to get holding registers
        data.val = GetHoldingRegisterValue_u16_driver(i); //put counter here for test
        buffer[3+dataCount*2]=data.buff[1];
        buffer[3+dataCount*2+1]=data.buff[0];
        dataCount++;
    }
    buffer[2]=dataCount*2;
    return dataCount*2+3;
}
uint8_t ResponseReadInputRegisters_04(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x04,  OK!!!!
{

	union{
	    uint16_t val ;
	    uint8_t buff[2];
		}data;
		uint8_t dataCount=0;
	   // ByteToAscii(, &sendBuffer[5], 1);
	    buffer[2]=(mb.limit - mb.start) * 2;
	    /* querying holding registers status */
	    for (int i = mb.start; i < mb.limit; i++)
	    {
	        //implement here what is needed to get holding registers
	        data.val = GetInputRegisterValue_u16_driver(i); //put counter here for test
	        buffer[3+dataCount*2]=data.buff[1];
	        buffer[3+dataCount*2+1]=data.buff[0];
	        dataCount++;
	    }
	    buffer[2]=dataCount*2;
	    return dataCount*2+3;

}
uint8_t ResponseForceSingleCoil_05(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x05, OK!!!!
{
    uint16_t coilID = 0;
    uint16_t value = 0;

    coilID = mb.start; //coil index

    value = mb.quantity; //coil value
    if (value == 0 || value == 65280)   // 0 or 1, 0000h or FF00h
    {
        SetCoilValue(coilID, value > 0);
    }


    return 6;
}

uint8_t ResponsePresetSingleRegister_06(uint8_t *buffer, uint8_t size, Modbus_t mb) //0x06, OK!!!!
{
    uint16_t registerID = 0;
    uint16_t value = 0;

    registerID = mb.start; //register index

    value = mb.quantity; //register value
    SetHoldingRegisterValue_u16_driver(registerID, value);
    for (int i = 5; i < 13; i++)
    {
        sendBuffer[i] = buffer[i];
    }

    return 6;
}

uint8_t ResponseForceMultipleCoils_15(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x0F, OK
{

    uint16_t i = 0;
    uint8_t coils = 0, k = 0, new_data_count = 0, tempAdr = 0;
    uint8_t index = 0, offset = 0;
    uint16_t parameters[20];
    uint8_t byte_count;

    //
    new_data_count =buffer[6];
    data_count = 7;
    //
    //    /* read new coils values */
    byte_count = 0;
    for (i = 0; i < new_data_count; i++)
    {
        coils =buffer[data_count];
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

uint8_t ResponsePresetMultipleRegisters_16(uint8_t *buffer, uint8_t size, Modbus_t mb) // 0x10, OK!!!
{

    uint16_t i = 0, reg_value = 0, sendIndex = 0;
    uint8_t byte_count;
    union{
    	    uint16_t val ;
    	    uint8_t buff[2];
    		}data;
    byte_count = buffer[6];
    sendIndex = 7;

    /* read and set new holding registers values */


    for (i = 0; i < byte_count; i++)
    {
        data.buff[1] = buffer[sendIndex++];
        data.buff[0] = buffer[sendIndex++];
        SetHoldingRegisterValue_u16_driver(mb.start++, data.val);

    }

    return 6;
}
