//#ifndef NTB_HWCTRL_H_
//#define NTB_HWCTRL_H_

//#include stm32f30x.h
#include <stm32f30x.h>
#include <stm32f30x_spi.h>
#include <stm32f30x_gpio.h>
#include <stm32f30x_rcc.h>
#include <system_stm32f30x.h>
//#include "ntb_cmnds.h"

#ifndef _SPI_H_
#define _SPI_H_

// Function prototypes
void ntb_init_hw();
void ntb_init_gpio();
void ntb_init_spidw();
void dw_init_irq();
void led_toggle_fast();
void led_toggle_medium();
void led_toggle_slow();
void led_on();
void led_off();
void dw_spi_ledtoggle(int ms);


#ifdef __cplusplus
{
	extern "C"	
}
#endif


/*
// Operating modes
#define NTB_MODE_NORMAL (0x00)
#define NTB_MODE_NOEXP  (0x01)
*/

// DW1000 SPI Settings

#define DW_SPI_PORT 	(GPIOA)
#define DW_SPI 			(SPI1)
#define DW_SPI_AF		(GPIO_AF_5)

#define DW_SCK_PIN (GPIO_Pin_5)
#define DW_SCK_PSOURCE (GPIO_PinSource5)

#define DW_MISO_PIN (GPIO_Pin_6)
#define DW_MISO_PSOURCE (GPIO_PinSource6)

#define DW_MOSI_PIN (GPIO_Pin_7)
#define DW_MOSI_PSOURCE (GPIO_PinSource7)

#define DW_SPI_CS_PORT 	(GPIOA)
#define DW_CS_PIN (GPIO_Pin_8)
#define DW_SPI_CLEAR_CS_FAST	{DW_SPI_CS_PORT->BRR = DW_CS_PIN;}
#define DW_SPI_SET_CS_FAST		{DW_SPI_CS_PORT->BSRR = DW_CS_PIN;}

#define DW_SPI_PERIPH (RCC_APB2Periph_SPI1)
#define DW_SPI_GPIOPERIPH (RCC_AHBPeriph_GPIOA)

// DW Reset Pin ---> PB8
#define DW_RESET_PORT (GPIOB)
#define DW_RESET_PIN (GPIO_Pin_8) // D4
#define DW_RESET_PERIPH (RCC_AHBPeriph_GPIOB)

// DW IRQ Pin --->  PB9
#define DW_IRQ_PORT 	(GPIOB)
#define DW_IRQ_PIN		(GPIO_Pin_9)
#define DW_IRQ_PERIPH (RCC_AHBPeriph_GPIOB)
#define DW_IRQ_LINE (EXTI_Line5)	// All GPIOs are on EXTI Lines 0-15
#define DW_IRQ_CHNL (EXTI9_5_IRQn) // lines 5-9
#define DW_IRQ_EXTI	  (EXTI_PortSourceGPIOB)
#define DW_IRQ_PSOURCE (EXTI_PinSource9)


#endif // NTB_HWCTRL_H_
