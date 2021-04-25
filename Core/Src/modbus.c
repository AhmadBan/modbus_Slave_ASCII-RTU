#include "modbus.h"
extern UART_HandleTypeDef huart2;
/* ASCII FRAME */
uint8_t  ascii_frame[ASCII_FRAME_SIZE]; // max size = 255
uint8_t  data_count = 0;
uint8_t  send_count = 0;
uint8_t broadcast = 0;

/* functions prototypes*/
static void convert(uint8_t *value,uint8_t noOfBytes){
#ifdef ASCII
	for(int i=0;i<noOfBytes;i++){

	}

#else


#endif

}
static Modbus_t getModbusInfo(uint8_t *buffer, uint8_t size){
	Modbus_t mb={0};

	    mb.start = AsciiToTwoByte(&buffer[5]);

	    mb.limit = mb.start + AsciiToTwoByte(&buffer[9]);
	    return mb;
}
uint8_t sendBuffer[ASCII_FRAME_SIZE];
void execute_modbus_command(uint8_t *buffer,uint8_t size)
{

    uint8_t sendIndex;
    Modbus_t mb;


    uint8_t is_me = 0, fun = 0, lrc = 0;

    if ( size < 9 )
    {     
        clear_frame(buffer);  // bad frame discard , minimun 9 characters for valid
        return;
    }



    size -= 2; //skip cr lf
    lrc = AsciiToByte(buffer[size-2], buffer[size-1]);
    size -= 2; //skip lrc
    fun = lrc_calc(buffer,size);
    if ( fun != lrc )
    {     
        clear_frame(buffer);  // bad LRC, frame discard
        return;
    }
     
    /*   frame ok  */
    is_me = AsciiToByte(buffer[1], buffer[2]);
    if ( is_me == 0 )
    {
        broadcast = 1;    
    }
    else
    {
        broadcast = 0;
        if ( DIR != is_me )
        {
            clear_frame(buffer);
            return;   
        }
    }


    mb=getModbusInfo(buffer, size);
    // response buffer start(1)+address(2)+protocol(2)
    for( sendIndex=0;sendIndex<5;sendIndex++){
    	sendBuffer[sendIndex]=buffer[sendIndex];//copy request header to response header
    }

    fun = AsciiToByte(buffer[3], buffer[4]);

    switch ( fun )
    {
        case 1:
        {                 
        	sendIndex=ResponseReadCoilsStatus_01(buffer,size,mb);
            break;        
        }
        case 2:
        {
        	sendIndex=ResponseReadInputStatus_02(buffer,size,mb);
            break;
        }
        case 3:
        {        
        	sendIndex=ResponseReadHoldingRegisters_03(buffer,size,mb);
            break;
        }
        case 4:
        {
            sendIndex=ResponseReadInputRegisters_04(buffer,size,mb);
            break;
        }
        case 5:
        {           
        	sendIndex=ResponseForceSingleCoil_05(buffer,size);
            break;
        }
        case 6:
        {
        	sendIndex=ResponsePresetSingleRegister_06(buffer,size);

            break;
        }
        case 15:
        {                     
            ResponseForceMultipleCoils_15(buffer,size,mb);
            break;            
        }        
        case 16:
        {              
            sendIndex=ResponsePresetMultipleRegisters_16(buffer,size,mb);
            break;            
        }        
        default: 
        {
            break;
        }
    }

    //generate lrc and fill lrcf at the end of send buffer
       gen_lrc(sendBuffer,sendIndex);
       sendIndex+=4;//add 4 bytes to size of buffer
       //transfer send buffer asynchronously by DMA
       tx_ascii_frame(sendBuffer,sendIndex);

}

void gen_lrc(uint8_t *buffer,uint8_t size)
{
    ByteToAscii(lrc_calc(buffer,size),&buffer[size],1);
    size+=2;
    
    buffer[size++] = CR;
    buffer[size++] = LF;
}
void clear_frame(uint8_t *buffer)
{    
    uint8_t i = 0;
    data_count = 0;
    send_count = 0;
    for ( i = 0; i < ASCII_FRAME_SIZE; i++ )
    {
    	buffer[i] = 0;
    }
}
void tx_ascii_frame(uint8_t *buffer,uint8_t size)
{
    if ( broadcast == 1 )
    {
        broadcast = 0;
        clear_frame(buffer);
        return ;    
    }
    HAL_UART_Transmit_DMA(&huart1, buffer, size);
}
uint8_t lrc_calc(uint8_t *buffer, uint8_t size)
{
    uint8_t result = 0, i = 0;
    for ( i = 1; i < size; i+=2 )
    {
        result += AsciiToByte(buffer[i], buffer[i+1]);
    }
    result = ~result;
    return ( result + 1 );
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
uint8_t ResponseReadCoilsStatus_01(uint8_t *buffer, uint8_t size,Modbus_t mb) // 0x01, OK
{

   uint8_t sendIndex=7,newDataCount=0,i,k,inputs;//update index to add data

	/* querying inputs status */
	for ( i =  mb.start; i < mb.limit; i += 8 )
	{
		k = 0;
		inputs = 0;
		while ( k < 8 && i + k < mb.limit)
		{
			uint8_t val =GetCoilValue(i + k);    // status of input i+k
			if ( val == 1 ) //input active
			{
				inputs |= (1 << k);
			}
			k++;
		}
		ByteToAscii(inputs,&sendBuffer[sendIndex],1);//change data to ASCII and filling send buffer correspondingly
		sendIndex+=2;
		newDataCount++;

	}
	ByteToAscii(newDataCount,&sendBuffer[5],1);
    return sendIndex;

}
uint8_t  ResponseReadInputStatus_02(uint8_t *buffer, uint8_t size,Modbus_t mb) // 0x02, OK!!!
{
	uint8_t sendIndex=7,newDataCount=0,i,k,inputs;//update index to add data

		/* querying inputs status */
		for ( i =  mb.start; i < mb.limit; i += 8 )
		{
			k = 0;
			inputs = 0;
			while ( k < 8 && i + k < mb.limit)
			{
				uint8_t val =GetInputRegisterValue_u16(i + k);    // status of input i+k
				if ( val == 1 ) //input active
				{
					inputs |= (1 << k);
				}
				k++;
			}
			ByteToAscii(inputs,&sendBuffer[sendIndex],1);//change data to ASCII and filling send buffer correspondingly
			sendIndex+=2;
			newDataCount++;

		}
		ByteToAscii(newDataCount,&sendBuffer[5],1);
	    return sendIndex;



}



uint16_t counter=1;
uint8_t ResponseReadHoldingRegisters_03(uint8_t *buffer, uint8_t size,Modbus_t mb)  // 0x03, OK!!!
{
	uint8_t sendIndex=7;//update index to add data
	uint16_t status=0;
	//length of data in bytes= (quantity - offset)*2 put length of bytes in 5th byte
	ByteToAscii((mb.limit-mb.start)*2,&sendBuffer[5],1);

    /* querying holding registers status */
    for ( int i = mb.start; i < mb.limit; i++ )
    {
        //implement here what is needed to get holding registers
    	status = GetHoldingRegisterValue_u16_driver(i);//put counter here for test

    	ByteToAscii(status,&sendBuffer[sendIndex],2);//change data to ASCII and filling send buffer correspondingly
        sendIndex+=4;//every two bytes of data require 4 byte ASCII code
    }
    return sendIndex;

    
}
uint8_t ResponseReadInputRegisters_04(uint8_t *buffer, uint8_t size,Modbus_t mb)  // 0x04,  OK!!!!
{
	uint8_t sendIndex=7;//update index to add data
		uint16_t status=0;
		//length of data in bytes= (quantity - offset)*2 put length of bytes in 5th byte
		ByteToAscii((mb.limit-mb.start)*2,&sendBuffer[5],1);

	    /* querying holding registers status */
	    for ( int i = mb.start; i < mb.limit; i++ )
	    {
	        //implement here what is needed to get holding registers
	    	status = GetInputRegisterValue_u16_driver(i);//put counter here for test
	    	ByteToAscii(status,&sendBuffer[sendIndex],2);//change data to ASCII and filling send buffer correspondingly
	        sendIndex+=4;//every two bytes of data require 4 byte ASCII code
	    }
	    return sendIndex;
}
uint8_t  ResponseForceSingleCoil_05(uint8_t *buffer, uint8_t size) // 0x05, OK!!!!
{
    uint16_t coilID = 0;
    uint16_t value =  0;

    coilID = AsciiToTwoByte(&buffer[5]);     //coil index

    value = AsciiToTwoByte(&buffer[9]);       //coil value
    if ( value == 0 || value == 65280 )     // 0 or 1, 0000h or FF00h
    {
        SetCoilValue(coilID, value > 0);
    }
    for(int i=5;i<13;i++){
    	sendBuffer[i]=buffer[i];
    }

    return 13;

}

uint8_t ResponsePresetSingleRegister_06(uint8_t *buffer, uint8_t size)  //0x06, OK!!!!
{
    uint16_t registerID = 0;
    uint16_t value =  0;

    registerID = AsciiToTwoByte(&buffer[5]);     //register index

    value = AsciiToTwoByte(&buffer[9]);          //register value
    SetHoldingRegisterValue_u16(registerID, value);
    for(int i=5;i<13;i++){
        	sendBuffer[i]=buffer[i];
        }

        return 13;
}

uint8_t  ResponseForceMultipleCoils_15(uint8_t *buffer, uint8_t size,Modbus_t mb) // 0x0F, OK
{

    uint16_t i = 0;
    uint8_t coils = 0, k = 0, new_data_count = 0, tempAdr = 0;
    uint8_t index = 0, offset = 0;
    uint16_t  parameters[20];
    uint8_t  byte_count;

//
    new_data_count = AsciiToByte(ascii_frame[13], ascii_frame[14] );
    data_count = 15;
//
//    /* read new coils values */
    byte_count = 0;
    for ( i = 0; i < new_data_count; i++ )
    {
        coils = AsciiToByte(ascii_frame[data_count], ascii_frame[data_count+1]);
        data_count += 2;
        parameters[byte_count++] = coils;
    }
//
//    /* force coils status */
    for ( i = mb.start; i < mb.limit; i++ )
    {
        tempAdr = i - mb.start;
        index = tempAdr / 8;
        offset = tempAdr % 8;
        k = ( parameters[index] & ( 1 << offset ) );
        SetCoilValue(i, k > 0);    // force status of coil i with k
    }


    return 13;
}

uint8_t ResponsePresetMultipleRegisters_16(uint8_t *buffer, uint8_t size,Modbus_t mb)   // 0x10, OK!!!
{

    uint16_t i = 0,  reg_value = 0,sendIndex=0;
//    signed char j = 0;
    uint8_t  byte_count;

//    start = AsciiToTwoByte();

//    cant = AsciiToTwoByte();
//    limit = start + cant;
//
    byte_count = AsciiToByte(buffer[13], buffer[14] );
    sendIndex = 15;
    for(i=5;i<13;i++)
    {
    	sendBuffer[i]=buffer[i];
    }
//    /* read and set new holding registers values */
    byte_count /= 2;


    for ( i = 0; i < byte_count; i++ )
    {
        reg_value = AsciiToTwoByte(&buffer[sendIndex]);
        SetHoldingRegisterValue_u16_driver(mb.start++, reg_value);
        sendIndex+=4;
    }


 return 13;
}
