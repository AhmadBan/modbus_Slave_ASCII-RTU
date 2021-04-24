/* slave address 1 - 255 */
#include "main.h"
#define DIR 0x01    //slave dir 5
/* data range */
#define MAX_COILS 1
#define MAX_INPUTS 1
#define MAX_HOLDING_REGISTERS 2*100 //must be multiple 2 bacause it is used to save 4-byte variables
#define MAX_INPUTS_REGISTERS 2*100

extern uint16_t holding_registers_array[MAX_HOLDING_REGISTERS];

/* functions prototypes */
uint8_t GetCoilValue(uint16_t adr);
void SetCoilValue(uint16_t adr, uint8_t value);
uint8_t GetInputValue(uint16_t adr);
uint16_t GetHoldingRegisterValue(uint16_t);
void SetHoldingRegisterValue(uint16_t, uint16_t);
uint16_t GetInputRegisterValue(uint16_t);
