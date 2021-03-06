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
 * This program will toggle on-board LED when the user switch is pressed.
 * Uses flags to switch between tasks and preemption is enabled.
 * A single task and interrupt is used in the program.
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


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"


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
void button_handler(void *params);
void led_task_handler(void *params);
void EXTI15_10_IRQHandler(void);

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

	prvSetupHardware();

	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Creating LED task
	xTaskCreate (led_task_handler,"LED-TASK",configMINIMAL_STACK_SIZE,NULL,1,NULL);

	//4. Start Scheduling
	vTaskStartScheduler();

	for(;;);

}


void led_task_handler(void *params)
{
	while(1)
	{
		if(button_status_flag == PRESSED)
		{
			//Turn ON
			GPIO_WriteBit(GPIOA, GPIO_Pin_5,Bit_SET);

		}
		else
		{
			//Turn OFF
			GPIO_WriteBit(GPIOA, GPIO_Pin_5,Bit_RESET);
		}
	}
}


void button_handler(void *params)
{

	button_status_flag ^= 1;


}


static void prvSetupHardware(void)
{
 //Setup UART
	prvSetupUART();
	prvSetupGPIO();

}

void printmsg(char *msg)
{
	for(uint32_t i=0; i<strlen(msg); i++)
	{
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);
		USART_SendData(USART2, msg[i]);
	}
}

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


void prvSetupGPIO(void)
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
	button_init.GPIO_OType = GPIO_OType_PP; //Not applicable so, even if it is PushPull there are no issues
	button_init.GPIO_Pin = GPIO_Pin_13;     //Button is connected to PC13
	button_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	button_init.GPIO_Speed = GPIO_Low_Speed;
	GPIO_Init(GPIOC, &button_init);

	//Interrupt configuration (Button is connected to PC13)
	//Enabling External Interrupt (Port C pin 13 so EXTI13 we have to configure)
	//SYSCFG Setting
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource13);

	//EXTI Line configuration
	EXTI_InitTypeDef exti_init;
	exti_init.EXTI_Line = EXTI_Line13;
	exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
	exti_init.EXTI_LineCmd = ENABLE;  // Enabling the interrupt line
	EXTI_Init(&exti_init);

	//NVIC Settings(IRQ Setting EXTI line is coming and hitting NVIC, so we should choose the IRQ number)
	//All interrupts from pin 10-15 has a single IRQ number (Position=40)

	NVIC_SetPriority(EXTI15_10_IRQn, 5);
	NVIC_EnableIRQ(EXTI15_10_IRQn);


}


//Interrupt handler function
void EXTI15_10_IRQHandler(void)
{

	traceISR_ENTER(); //For segger to trace ISR entry
	//1. Clear the interrupt pending bit of the EXTI Line(13)
	EXTI_ClearITPendingBit(EXTI_Line13);
	button_handler(NULL);
	traceISR_EXIT(); // For segger to trace ISR exit
}
