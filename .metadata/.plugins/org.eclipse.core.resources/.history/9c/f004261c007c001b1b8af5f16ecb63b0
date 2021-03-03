/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"


TaskHandle_t xTaskHandle1=NULL;
TaskHandle_t xTaskHandle2=NULL;
			
//Function prototypes
static void prvSetupHardware(void);
void printmsg(char *msg);
static void prvSetupUART(void);
void prvSetupGPIO(void);
void vtask_led_handler(void *params);
void vtask_button_handler(void *params);
void  rtos_delay(uint32_t delay_in_ms);

//Global variable section
char usr_msg[200]={0};


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

	sprintf(usr_msg,"Task Notification API Project");
	printmsg(usr_msg);

	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	xTaskCreate(vtask_led_handler,"LED-Task",500,NULL,2,&xTaskHandle1 ); //500 is given as stack memory as we are using APIs
	xTaskCreate(vtask_button_handler,"Button-Task",500,NULL,2,&xTaskHandle2 );


	//4. Start the scheduler
	vTaskStartScheduler();

	for(;;);

}

void vtask_led_handler(void *params)
{
	while(1)
	{
		//Waiting until Notification is received from Button Task
		if ( xTaskNotifyWait(0,0,NULL,portMAX_DELAY) == pdTRUE)
		{
			//Notification is received
			GPIO_ToggleBits(GPIOA, GPIO_Pin_5);
			sprintf(usr_msg, "Notification received \r\n");
			printmsg(usr_msg);
		}

	}
}

void vtask_button_handler(void *params)
{
	while(1)
	{
		if(!GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13))
		{
			//Button is pressed
			//Delay for button debouncing 100ms
			rtos_delay(100);

			//Send notification to LED task
			xTaskNotify(xTaskHandle1, 0x0, eNoAction);
		}
	}
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

void  rtos_delay(uint32_t delay_in_ms)
{
	uint32_t current_tick = xTaskGetTickCount();
	//Converting mili-second delay to ticks
	uint32_t delay_in_ticks = (delay_in_ms * configTICK_RATE_HZ)/1000;

	while(xTaskGetTickCount() < (current_tick + delay_in_ticks));

}
