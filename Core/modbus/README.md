# Getting Started

To use this library do the following 
 1. add include file to your main.c file
   ```c
#include "modbus.h"
   ```
 2. initialize modbus inside your main.c file 
   
   ```c
   main(){
    ... 
    initModbus();
    ...
    while(1){
        ...
    }

   }
   ```


 3. run your procedure to get request from master and fill preallocated buffer **mbBuffer** for example in STM32HAL library
   

```c
HAL_UARTEx_ReceiveToIdle_DMA(&huart1, mbBuffer, ASCII_FRAME_SIZE);

```
4. call **execute_modbus_command(mbBuffer, Size)** in your code when you get a request from master and get the size of response buffer and then use filled **sendBuffer** buffer to respond master. for example 


```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  uint8_t responseLength;
  responseLength=execute_modbus_command(mbBuffer, Size);
  if(responseLength>0)
	  HAL_UART_Transmit_DMA(&huart1, sendBuffer, responseLength);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, mbBuffer, ASCII_FRAME_SIZE);
}

```

# Working with the different data types
By default modbus save data in 16-bit variables. Thus to save and load 32-bit data one must use two 16-bit variables.

To manage that there are some api inside library to send and receive 32-bit library

```c
//Get holding registers
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
```

notice that Adresses for 32-bit variable are completly different with 16-bit variable. for example address 20 for a 32-bit variable equals to R39 and R40 MSB and LSB respectively
so the developer is in charge with choosing propper addresses for different variables so there is no address conflict in the system.

