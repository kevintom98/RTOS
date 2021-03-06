/*
 * Author: Kevin Tom (https://sites.google.com/view/kevintom)
 *
 * Microcontroller : STM32F446RE-NUCLEO
 * 					 ARM CORTEX M4 core at 180 MHz
 * 					 512 Kb Flash
 * 					 128 Kb SRAM
 *
 * Description
 * ```````````
 * The code is basic "Hello World" application using built on FreeRTOS,
 * developed for learning task creation, scheduling, etc.
 *
 * SEMI-HOSTING is enabled for testing the code.
 * UART is configured for observing the output on COM port.
 *
 * UART Details
 * ````````````
 * Used				 : 	USART2
 * Pins				 :	PA2 (TX)
 * 		 				PA3 (RX)
 * Baudrate 		 : 	115200
 * No. Of Stop Bits  : 	1
 *
 *
 */

//Including library files
#include <stdio.h>
#include <stdint.h>
#include <string.h>

//Library files related to STM32 and FreeRTOS
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

//Initializing Task handle
TaskHandle_t xTaskHandle1=NULL;
TaskHandle_t xTaskHandle2=NULL;


//Task function prototype
void vTask1_handler(void *params);
void vTask2_handler(void *params);


#ifdef USE_SEMIHOSTING
//Used for hosting
extern void initialise_monitor_handles();
#endif


//Function prototypes
static void prvSetupHardware(void);
static void prvSetupUART(void);


void printmsg(char *msg);



//Macros section
#define TRUE 1
#define FALSE 0

#define AVAILABLE TRUE
#define NOT_AVAILABLE FALSE

//Global variables
char usr_msg[200]={0};
uint8_t UART_ACCESS_KEY = AVAILABLE;

int main(void)
{

#ifdef USE_SEMIHOSTING
	initialise_monitor_handles();
	printf("Hello world from main \n");
#endif

	//For enabling cycle counter(SEGGER)
	DWT->CTRL |= (1<<0);


	//1. DeIntialzing the 180MHz clock to default(disabling the PLL), because we do not need such high speed.
	//HSI ON, PLL OFF,HSE OFF, system clock=16Mhz, cpu_clock = 16Mhz
	RCC_DeInit();


	//2.Update the SystemCoreClock variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	sprintf(usr_msg,"This is hello world application starting\r\n");
	printmsg(usr_msg);

	//Start recording for SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Creating two tasks
	xTaskCreate(vTask1_handler,"Task-1",configMINIMAL_STACK_SIZE,NULL,2,&xTaskHandle1 );
	xTaskCreate(vTask2_handler,"Task-2",configMINIMAL_STACK_SIZE,NULL,2,&xTaskHandle2 );

	//4. Start the scheduler
	vTaskStartScheduler();


	for(;;);
}




void vTask1_handler(void *params)
{
	//Always the task handler should be in infinite loop
	while(1)
	{
		//Proceed only if UART KEY is AVAILABLE (It will only become AVAILABLE when Transmission is complete)
		if(UART_ACCESS_KEY == AVAILABLE)
		{
			UART_ACCESS_KEY = NOT_AVAILABLE; //Making Key NOT_AVAILABLE until transmission is complete
			printmsg("Hello world from task-1\r\n");
			UART_ACCESS_KEY = AVAILABLE; //Making KEY available after transmission
			SEGGER_SYSVIEW_Print("Task-1 is yielding");
			traceISR_EXIT_TO_SCHEDULER(); //Used for displaying PendSV black line in SEGGER
			taskYIELD();   //Manual switching
		}
	}
}

void vTask2_handler(void *params)
{
	while(1)
	{
		if(UART_ACCESS_KEY == AVAILABLE)
		{
			UART_ACCESS_KEY = NOT_AVAILABLE; //Making Key NOT_AVAILABLE until transmission is complete
			printmsg("Hello world from task-2\r\n");
			UART_ACCESS_KEY = AVAILABLE; //Making KEY available after transmission
			SEGGER_SYSVIEW_Print("Task-2 is yielding");
			traceISR_EXIT_TO_SCHEDULER();  //Used for displaying PendSV black line in SEGGER
			taskYIELD(); //Manual switching
		}
	}
}


static void prvSetupUART(void)
{

	//Define GPIO and UART class variables
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


//Function for calling hardware setup functions
static void prvSetupHardware(void)
{
 //Setup UART
	prvSetupUART();
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
