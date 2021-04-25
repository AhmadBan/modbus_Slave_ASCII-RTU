/* slave address 1 - 255 */
#include "main.h"
#define DIR 0x01    //slave dir 5
/* data range */



#define MAX_COILS 1
#define MAX_INPUTS 1
#define MAX_HOLDING_REGISTERS 50 //must be multiple 2 bacause it is used to save 4-byte variables
#define MAX_INPUTS_REGISTERS 50





extern uint16_t holding_registers_array[MAX_HOLDING_REGISTERS];

/* functions prototypes */
uint8_t GetCoilValue(uint16_t adr);
void SetCoilValue(uint16_t adr, uint8_t value);

uint16_t GetHoldingRegisterValue_u16_driver(uint16_t adr);
uint16_t GetHoldingRegisterValue_u16(uint16_t adr);
int16_t GetHoldingRegisterValue_s16(uint16_t adr);
//care must be taken to pass correct address because it returns 32-bit unsigned
uint32_t GetHoldingRegisterValue_u32(uint16_t adr);
int32_t GetHoldingRegisterValue_s32(uint16_t adr);
float GetHoldingRegisterValue_f32(uint16_t adr);


void SetHoldingRegisterValue_u16(uint16_t adr, uint16_t value);
void SetHoldingRegisterValue_s16(uint16_t adr, int16_t value);
void SetHoldingRegisterValue_u32(uint16_t adr, uint32_t value);
void SetHoldingRegisterValue_s32(uint16_t adr, int32_t value);
void SetHoldingRegisterValue_f32(uint16_t adr, float value);

uint16_t GetInputRegisterValue_u16_driver(uint16_t adr);
uint16_t GetInputRegisterValue_u16(uint16_t adr);
int16_t GetInputRegisterValue_s16(uint16_t adr);
//care must be taken to pass correct address because it returns 32-bit unsigned
uint32_t GetInputRegisterValue_u32(uint16_t adr);
int32_t GetInputRegisterValue_s32(uint16_t adr);
float GetInputRegisterValue_f32(uint16_t adr);


void SetInputRegisterValue_u16(uint16_t adr, uint16_t value);


void SetInputRegisterValue_s16(uint16_t adr, int16_t value);

void SetInputRegisterValue_u32(uint16_t adr, uint32_t value);

void SetInputRegisterValue_s32(uint16_t adr, int32_t value);

void SetInputRegisterValue_f32(uint16_t adr, float value);
