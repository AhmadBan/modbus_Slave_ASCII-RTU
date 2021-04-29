
#ifndef MODBUS_H
#define MODBUS_H


#include "main.h"
/*******************************************************************/
//uncomment one of RTU or ASCII and compile library
#define RTU
//#define ASCII
/*******************************************************************/

#define DEVICE_ID 0x01 //slave dir 5
/*******************************************************************/
/* data range */

#define MAX_COILS 10
#define MAX_INPUTS 10
#define MAX_HOLDING_REGISTERS 50 //must be multiple 2 bacause it is used to save 4-byte variables
#define MAX_INPUTS_REGISTERS 50
#define ASCII_FRAME_SIZE MAX_HOLDING_REGISTERS * 4 + 20 // In worst case if master tries to read all of the register with one command, sending buffer must have length_of_register*4+header(header is less than 20 bytes)


/****************variables*******************************************/
extern uint8_t mbBuffer[ASCII_FRAME_SIZE];
extern uint8_t sendBuffer[ASCII_FRAME_SIZE];

/****************functions prototypes********************************/

/* functions prototypes */
uint8_t GetInputStatusValue(uint16_t adr);
void SetInputStatusValue(uint16_t adr, uint8_t value);
uint8_t GetCoilValue(uint16_t adr);
void SetCoilValue(uint16_t adr, uint8_t value);
//Get Holding Registers

uint16_t GetHoldingRegisterValue_u16(uint16_t adr);
int16_t GetHoldingRegisterValue_s16(uint16_t adr);
uint32_t GetHoldingRegisterValue_u32(uint16_t adr);
int32_t GetHoldingRegisterValue_s32(uint16_t adr);
float GetHoldingRegisterValue_f32(uint16_t adr);
//Set Holding Registers

void SetHoldingRegisterValue_u16(uint16_t adr, uint16_t value);
void SetHoldingRegisterValue_s16(uint16_t adr, int16_t value);
void SetHoldingRegisterValue_u32(uint16_t adr, uint32_t value);
void SetHoldingRegisterValue_s32(uint16_t adr, int32_t value);
void SetHoldingRegisterValue_f32(uint16_t adr, float value);
//Get Input Registers

uint16_t GetInputRegisterValue_u16(uint16_t adr);
int16_t GetInputRegisterValue_s16(uint16_t adr);
uint32_t GetInputRegisterValue_u32(uint16_t adr);
int32_t GetInputRegisterValue_s32(uint16_t adr);
float GetInputRegisterValue_f32(uint16_t adr);
//Set Input Registers
void SetInputRegisterValue_u16(uint16_t adr, uint16_t value);
void SetInputRegisterValue_s16(uint16_t adr, int16_t value);
void SetInputRegisterValue_u32(uint16_t adr, uint32_t value);
void SetInputRegisterValue_s32(uint16_t adr, int32_t value);
void SetInputRegisterValue_f32(uint16_t adr, float value);


void initModbus(void);
/* functions prototypes*/
uint8_t execute_modbus_command(uint8_t *buffer, uint8_t size);
//void initComPortCallback(uint8_t* fillBuff);

#endif
