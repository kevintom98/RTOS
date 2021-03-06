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
 * Initially the button will be blink at 1Hz rate, after pressing the button
 * the toggling rate increases to 2Hz. The first task will get deleted and
 * second task with 2Hz rate get executed.
 *
 * Tasks:
 * Delete Task - Gets deleted when switch is pressed (1Hz toggling rate and has highest priority)
 * Led Blink Task - Gets executed when Delete Task gets deleted (2Hz toggling rate and has low priority than Delete Task)
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

//Header files
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
			
//Function prototypes
static void prvSetupHardware(void);
static void prvSetupUART(void);
static void prvSetupGPIO(void);
void printmsg(char *msg);
void vtask_led_delete_handler(void *params);
void vtask_led_handler(void *prams);
void  rtos_delay(uint32_t delay_in_ms);

TaskHandle_t xTaskHandle1=NULL;
TaskHandle_t xTaskHandle2=NULL;


//Array declaration for printing over UART
char usr_msg[100];


int main(void)
{
	//For enabling cycle counter(SEGGER)
	DWT->CTRL |= (1<<0);

	//DeIntialzing the 180MHz clock to default(disabling the PLL), because we do not need such high speed.
	//HSI ON, PLL OFF,HSE OFF, system clock=16Mhz, cpu_clock = 16Mhz
	RCC_DeInit();

	//Update the SystemCoreClock variable
	SystemCoreClockUpdate();

	//Calling hardware setup function setup
	prvSetupHardware();

	sprintf(usr_msg,"Task Deletion API Project");
	printmsg(usr_msg);

	//SEGGER System View configuration
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//Creating tasks
	xTaskCreate(vtask_led_delete_handler, "Delete Task", 500, NULL, 2, &xTaskHandle1);
	xTaskCreate(vtask_led_handler, "Led Blink Task", 500, NULL, 1, &xTaskHandle2);

	//Starting the scheduler
	vTaskStartScheduler();

	for(;;);
}


//This task gets deleted after pressing the switch
void vtask_led_delete_handler(void *params)
{
	sprintf(usr_msg, "Delete handler task is running \r\n");
	printmsg(usr_msg);

	while(1)
	{

		if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13))
		{
			rtos_delay(1000);
			GPIO_ToggleBits(GPIOA,GPIO_Pin_5);
		}
		else
		{
			sprintf(usr_msg, "Deleting task \r\n");
			printmsg(usr_msg);
			vTaskDelete(NULL);  //Deleting the current task
		}
	}
}


//This task runs after pressing the switch
void vtask_led_handler(void *prams)
{
	sprintf(usr_msg, "Led task is running \r\n");
	printmsg(usr_msg);

	while(1)
	{
		//rtos_delay(200);
		vTaskDelay(200); //This function will run Idle task (Which helps in deleting previous task, as task will only be deleted when Idel task runs)
		GPIO_ToggleBits(GPIOA,GPIO_Pin_5);  //Toggling the bit
	}


}

//Function for calling different hardware setup functions
static void prvSetupHardware(void)
{
	prvSetupUART();
	prvSetupGPIO();
}

static void prvSetupUART(void)
{

	GPIO_InitTypeDef gpio_uart_pins;
	USART_InitTypeDef uart2_init;

	//Enabling UART and GPIO peripheral port clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	//PA2 is USART2_TX and PA3 is USART2_RX(from data-sheet(alternate pin section of data-sheet))
	memset(&gpio_uart_pins,0,sizeof(gpio_uart_pins));
	gpio_uart_pins.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_1;
	gpio_uart_pins.GPIO_Mode = GPIO_Mode_AF;
	gpio_uart_pins.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_uart_pins);

	//Alternate Function Mode setting for pins
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); //PA2
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //PA3

	//Enabling UART
	memset(&uart2_init,0,sizeof(uart2_init));
	uart2_init.USART_BaudRate = 115200;
	uart2_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart2_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	uart2_init.USART_Parity = USART_Parity_No;
	uart2_init.USART_StopBits = USART_StopBits_1;
	uart2_init.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &uart2_init);


	USART_Cmd(USART2, ENABLE);

}


static void prvSetupGPIO(void)
{

	//Please note that this function is board specific
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); //For enable interrupt(SYSCFG)

	GPIO_InitTypeDef led_init, button_init;
	led_init.GPIO_Mode = GPIO_Mode_OUT;
	led_init.GPIO_OType = GPIO_OType_PP;
	led_init.GPIO_Pin = GPIO_Pin_5;  //LED is connected to PA5
	led_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	led_init.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOA, &led_init);

	button_init.GPIO_Mode = GPIO_Mode_IN;
	button_init.GPIO_OType = GPIO_OType_PP; //Not applicable so, even if it is PushPull there is no problem
	button_init.GPIO_Pin = GPIO_Pin_13;     //Button is connected to PC13
	button_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	button_init.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOC, &button_init);
}



//Function to print any message through UART(Mainly used for debugging)
void printmsg(char *msg)
{
	for(uint32_t i=0; i<strlen(msg); i++)
	{
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);
		USART_SendData(USART2, msg[i]);
	}
}

//Custom delay function
void  rtos_delay(uint32_t delay_in_ms)
{
	uint32_t current_tick = xTaskGetTickCount();
	//Converting milli-second delay to ticks
	uint32_t delay_in_ticks = (delay_in_ms * configTICK_RATE_HZ)/1000;
	while(xTaskGetTickCount() < (current_tick + delay_in_ticks));
}
