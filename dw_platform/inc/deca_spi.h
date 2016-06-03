#ifndef DECA_SPI_H_
#define DECA_SPI_H_

#include <stdint.h>
#include <stm32f30x.h>	// Support for the STM32F303RBT6 MCU
#include <stm32f30x_spi.h>	// Support for the STM32F303RBT6 MCU
//#include "stm32f4xx.h"
//#include "stm32f4xx_spi.h"
#include <ntb_hwctrl.h>	// board specific DW SPI pins defintions

// Busy waits
#define CLOCK_TICKS_PER_USEC (72) // for 72 MHz internal clock after PLL multiplication

// Function prototypes
void dw_spi_init();
uint8_t dw_spi_transferbyte(uint8_t byte);
int dw_spi_sendpacket(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, const uint8_t* body);
int dw_spi_readpacket(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, uint8_t* body);
void dw_spi_busysleep(uint32_t usec);
void dw_sleep_usec(uint32_t usec);
void dw_sleep_msec(uint32_t msec);

// Debugging function prototypes
void spi_send8(SPI_TypeDef* SPIx, uint8_t data);
uint8_t spi_read8(SPI_TypeDef* SPIx);

// New prototype of function made for debugging 
//uint8_t spi_sendbyte(SPI_TypeDef* SPIx, uint8_t databyte);

#endif // DECA_SPI_H_