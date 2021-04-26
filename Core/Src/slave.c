
#include "slave.h"
/* internal slave memory data */
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

/* functions codes */

/* coils read-write operations */
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

uint8_t GetInputStatusValue(uint16_t adr)
{
    uint8_t res = 0, i = 0, d = 0;
    if (adr < 0 || adr > MAX_COILS * 8)
    {
        return 0;
    }
    i = adr / 8;
    d = adr % 8;
    res = (inputs_array[i] & (1 << d));
    return res > 0;
}

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



/* holding registers read-write operations */

uint16_t GetHoldingRegisterValue_u16_driver(uint16_t adr)
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

void SetHoldingRegisterValue_u16_driver(uint16_t adr, uint16_t value)
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

void SetHoldingRegisterValue_u32(uint16_t adr, uint32_t value)
{

    if (adr < 0 || adr > MAX_HOLDING_REGISTERS || value < 0)
    {
        return; // invalid register address
    }
    else
    {
        //    	holdingRegister.u16_holding_registers_array[adr*2+1]=(value&0xffff0000)>>16;
        //    	holdingRegister.u16_holding_registers_array[adr*2]=(value&0xffff);
        holdingRegister.u32_holding_registers_array[adr] = value;
    }
}

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

//input
uint16_t GetInputRegisterValue_u16_driver(uint16_t adr)
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

/* inputs read-only operations */
uint8_t GetInputValue(uint16_t adr)
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
