# Example STM32F051R8T6 Getting Started

This is an example of using modbus library which is a project customized for STMkit in STM32CUBEIDE 

To configure UART settings use cube as shown below

![picture to configure UART settings](./Picture/CubeIDe.bmp)



To use modbus library, It is initialized with **initModbus** function like below

```c

#include "modbus.h"

int main(void)
{
    //some initialization here
.
.
.

  /* USER CODE BEGIN 2 */
    //initialize modbus
    initModbus();
    //start getting data from uart by DMA
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, mbBuffer, ASCII_FRAME_SIZE);
 
    //add your coustom code here

  /* USER CODE END 2 */
    while(1){

    }

}
```

after initialization Microcontroller tries to get data from master by using DMA and then after getting data it calls a callback function 

```c

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    uint8_t responseLength;
    //pass received buffer **mbBuffer** and Size of it to process function "execute_modbus_command" then it fills response buffer "sendBuffer" and returns its size 
    responseLength=execute_modbus_command(mbBuffer, Size);
    //transmit response buffer "sendBuffer" to output port
    if(responseLength>0)//check if size is bigger than 0
        HAL_UART_Transmit_DMA(&huart1, sendBuffer, responseLength);
    //initialize UART port again to receive request from master
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, mbBuffer, ASCII_FRAME_SIZE);

}


//this callback reset DMA connection in case of any error with UART

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, mbBuffer, ASCII_FRAME_SIZE);

}

```
as you can see it calls function **execute_modbus_command** and passes buffer of data to it everything else is done by library
