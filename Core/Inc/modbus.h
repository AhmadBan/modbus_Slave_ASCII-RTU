#include "main.h"
#include <slave.h>
#include <ascii.h>

/* ASCII FRAME size */

#define ASCII_FRAME_SIZE MAX_HOLDING_REGISTERS*4+20   // In worst case if master try to read all of the register with one command send buffer must have length_of_register*4+header(which is less than 20 bytes)

/* ASCII FRAME END CHARS */
#define CR 0x0D
#define LF 0x0A         
typedef struct {
	uint8_t start;
	uint8_t limit;
}Modbus_t ;


/* functions prototypes*/
void execute_modbus_command(uint8_t *buffer,uint8_t size);
void gen_lrc(uint8_t *buffer,uint8_t size);
void clear_frame(uint8_t *buffer);
uint8_t lrc_calc(uint8_t *buffer,uint8_t size);
void tx_ascii_frame(uint8_t *buffer, uint8_t size);

uint8_t ResponseReadCoilsStatus_01(uint8_t *buffer, uint8_t size,Modbus_t mb);
uint8_t ResponseReadInputStatus_02(uint8_t *buffer, uint8_t size,Modbus_t mb);
uint8_t ResponseReadHoldingRegisters_03(uint8_t *buffer, uint8_t size,Modbus_t mb);
uint8_t ResponseReadInputRegisters_04(uint8_t *buffer, uint8_t size,Modbus_t mb);
uint8_t ResponseForceSingleCoil_05(uint8_t *buffer, uint8_t size);
uint8_t ResponsePresetSingleRegister_06(uint8_t *buffer, uint8_t size);
uint8_t  ResponseForceMultipleCoils_15(uint8_t *buffer, uint8_t size,Modbus_t mb);
uint8_t ResponsePresetMultipleRegisters_16(uint8_t *buffer, uint8_t size,Modbus_t mb);
