/*
 *
 *
 * Microcontroller : STM32F446RE-NUCLEO
 * 					 ARM CORTEX M4 core at 180 MHz
 * 					 512 Kb Flash
 * 					 128 Kb SRAM
 *
 * Description
 * ```````````
 * This program will toggle on/off on-board LED when the user switch is pressed.
 * Uses flags to switch between tasks and preemption is enabled.
 *
 * UART Details
 * ````````````
 * Used				 : 	USART2
 * Pins				 :	PA2 (TX)
 * 		 				PA3 (RX)
 * Baudrate 		 : 	115200
 * No. Of Stop Bits  : 	1
 *
 * GPIO
 * ````
 * Used 			 : GPIOA, GPIOC
 * Pins 			 : PA5, PC13
 *
 */


//Including libraries
#include <stdio.h>
#include <stdint.h>
#include <string.h>

//Library files related to STM32 and FreeRTOS
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"



//Macro section
#define TRUE 1
#define FALSE 0

#define NOT_PRESSED FALSE
#define PRESSED TRUE


//Function prototypes
static void prvSetupHardware(void);
void printmsg(char *msg);
static void prvSetupUART(void);
void prvSetupGPIO(void);

//Task prototypes
void button_task_handler(void *params);
void led_task_handler(void *params);

//Global variable
uint8_t button_status_flag = NOT_PRESSED;



int main(void)
{
	//For enabling cycle counter(SEGGER)
	DWT->CTRL |= (1<<0);


	//1. DeIntialzing the 180MHz clock to default(disabling the PLL), because we do not need such high speed.
	//HSI ON, PLL OFF,HSE OFF, system clock=16Mhz, cpu_clock = 16Mhz
	RCC_DeInit();


	//2.Update the SystemCoreClock variable
	SystemCoreClockUpdate();

	//Calling function to setup hardware like UART and GPIO
	prvSetupHardware();

	//SEGGGER SystemView Configuration
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Creating LED task
	xTaskCreate (led_task_handler,"LED-TASK",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate (button_task_handler,"BUTTON-TASK",configMINIMAL_STACK_SIZE,NULL,1,NULL);


	//4. Start Scheduling
	vTaskStartScheduler();

	for(;;);

}



//Task handler function for switching on LED according to button status
void led_task_handler(void *params)
{
	const TickType_t xDelay = pdMS_TO_TICKS(250);

	while(1)
	{
		if(button_status_flag == PRESSED)
		{
			//Toggle ON/OFF the LED
			GPIO_ToggleBits(GPIOA, GPIO_Pin_5);
			vTaskDelay(xDelay);
		}
		else
		{
			//Turn OFF the LED
			GPIO_WriteBit(GPIOA, GPIO_Pin_5,Bit_RESET);
		}
	}
}



//Task handler function for checking if the button is pressed or not
void button_task_handler(void *params)
{
	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13))
		{
			//Button is pressed
			button_status_flag = NOT_PRESSED;
		}
		else
		{
			//Button is not pressed
			button_status_flag = PRESSED;
		}
	}
}



//Function for calling hardware port setup files
static void prvSetupHardware(void)
{
	//Setup UART
	prvSetupUART();
	//Setup GPIO
	prvSetupGPIO();

}


//Function for printing message over UART
void printmsg(char *msg)
{
	for(uint32_t i=0; i<strlen(msg); i++)
	{
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);
		USART_SendData(USART2, msg[i]);
	}
}


//Function for setting up UART
static void prvSetupUART(void)
{

	GPIO_InitTypeDef gpio_uart_pins;
	USART_InitTypeDef uart2_init;

	//1. Enable UART2 and GPIOA (Because COM port uses UART2 of nucleo board) peripheral port
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	//PA2 is USART2_TX and PA3 is USART2_RX(from datasheet(alternate pin section of datasheet))

	//2. Alternate function enabling on PA2 and PA3
	//Setting each and every member of the structure
	memset(&gpio_uart_pins,0,sizeof(gpio_uart_pins));
	gpio_uart_pins.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	gpio_uart_pins.GPIO_Mode = GPIO_Mode_AF;
	gpio_uart_pins.GPIO_PuPd =  GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_uart_pins);

	//3.AF Mode setting for pins
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); //PA2
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //PA3


	//4. Parameter initialization of UART
	memset(&uart2_init,0,sizeof(uart2_init));
	uart2_init.USART_BaudRate = 115200;
	uart2_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart2_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	uart2_init.USART_Parity = USART_Parity_No;
	uart2_init.USART_StopBits = USART_StopBits_1;
	uart2_init.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &uart2_init);


	//5. Enable the UART2 peripheral
	USART_Cmd(USART2, ENABLE);

}


//Function for setting up GPIO
void prvSetupGPIO(void)
{
	//Please note that this function is board specific

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef led_init, button_init;
	led_init.GPIO_Mode = GPIO_Mode_OUT;
	led_init.GPIO_OType = GPIO_OType_PP;
	led_init.GPIO_Pin = GPIO_Pin_5;  //LED is connected to PA5
	led_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	led_init.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOA, &led_init);

	button_init.GPIO_Mode = GPIO_Mode_IN;
	button_init.GPIO_OType = GPIO_OType_PP; //Not applicable so, even if it is PushPull there are no issues
	button_init.GPIO_Pin = GPIO_Pin_13;     //Button is connected to PC13
	button_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	button_init.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOC, &button_init);

}

