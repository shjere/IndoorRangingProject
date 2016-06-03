#include "deca_spi.h"
#include "ntb_hwctrl.h"
#include <stdint.h>
#include "stm32f30x.h"
#include "stm32f30x_spi.h"
//#include "spi.h"
//#include "stm32f4xx.h"
//#include "stm32f4xx_spi.h"

// Function made for debugging 
/*
uint8_t spi_sendbyte(SPI_TypeDef* SPIx, uint8_t databyte)
{
	while( !(SPIx->SR & SPI_I2S_FLAG_TXE) ); // wait until transmit complete
	SPIx->DR = databyte;

	// Debugging: LED toggle
  	led_toggle_medium();
	
	
	while( !(SPIx->SR & SPI_I2S_FLAG_RXNE) ); // wait until receive complete
	while( SPIx->SR & SPI_I2S_FLAG_BSY ); // wait until SPI is not busy anymore
	return SPIx->DR; // return received data from SPI data register
}
*/

// Debugging: Send 8 bits over SPI MOSI pin

void spi_send8(SPI_TypeDef* SPIx, uint8_t data)
{
	
	// Wait until previous transfer is complete
	while (!(SPIx->SR & SPI_I2S_FLAG_TXE));

	// Transfer the byte of data
	SPI_SendData8(SPIx, data);
	//SPIx->DR = data;


	// Debugging: Transfer 2 bytes instead
	//SPI_I2S_SendData16(SPIx, data);
	
}


// Debugging: Receive 8 bits over SPI MISO pin

uint8_t spi_read8(SPI_TypeDef* SPIx)
{
	// Wait until all previous data has been transmitted/ received
	while (SPIx->SR & SPI_I2S_FLAG_BSY);

	// Wait for any data on MISO pin to be received
	while (!(SPIx->SR & SPI_I2S_FLAG_RXNE));

	// Receive a byte of data
	return SPI_ReceiveData8(SPIx);
}


// Debugging: Send 8 bits of data over SPI peripheral as done in libopencm3
/*
void spi_send8(uint32_t spi, uint8_t data)
{
	// Wait for a transfer to finish
	while(!(SPI_SR(spi) & SPI_SR_TXE));

	// Write data (8 or 16 bits, depending on DFF) into DR
	SPI_DR8(spi) = data;
}
*/

// Debugging: Receive 8 bits over SPI as done in libopencm3
/*
uint8_t spi_read8(uint32_t spi)
{
	// Wait for transfer to finish
	while(!(SPI_SR(spi) & SPI_SR_RXNE));

	// Read the data (8 or 16 bits, depending on DFF bit) from DR
	return SPI_DR8(spi);
}
*/

uint8_t dw_spi_transferbyte(uint8_t byte)
{	
	
	// Wait until all previous data has been transmitted/ received on SPI1
	while (SPI1->SR & SPI_I2S_FLAG_BSY);

	
	// Send out a byte of data
	//spi_send8(DW_SPI, byte);

	// Debugging: LED toggle
	//led_toggle_medium();

	
	// Receive a byte of data 
	//return spi_read8(DW_SPI);

	
	
	// Waiting until TX FIFO is empty
	/*
	while (SPI_GetTransmissionFIFOStatus(DW_SPI) != SPI_TransmissionFIFOStatus_Empty)
    {}
	*/

	//while(!SPI_I2S_GetFlagStatus(DW_SPI, SPI_I2S_FLAG_TXE));
	
	// Wait for current transmission on MOSI to finish
	while (!(DW_SPI->SR & SPI_I2S_FLAG_TXE));

	// Transfer a byte of data 
	SPI_SendData8(DW_SPI, byte);

	// Debugging: send a byte out on the MOSI pin
	//spi_sendbyte(DW_SPI, byte);

	
	// Debugging: Using SPI send function from libopencm3
	//spi_send8(DW_SPI, byte);

	// 16-bit data transfer 
	/*
	uint16_t byte16 = (uint16_t)byte;
	SPI_I2S_SendData16(DW_SPI, byte16);
	*/

	/*
	uint8_t spi_success;
	spi_success = spi_sendbyte(DW_SPI, byte);
	*/

	while (!(DW_SPI->SR & SPI_I2S_FLAG_RXNE));
	return SPI_ReceiveData8(DW_SPI);


	// Debugging: Send a dummy byte 0x00 out and read a byte received on the MISO pin 
	//return spi_sendbyte(DW_SPI, 0x00);

	// Debugging: Using 8 bit SPI read function from libopencm3
	//return spi_read8(DW_SPI);

	//return SPI_I2S_ReceiveData16(DW_SPI);

}

int dw_spi_sendpacket(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, const uint8_t* body)
{
	int i;
	uint8_t tmp;

	// CS Low and wait a bit
	GPIO_ResetBits(DW_SPI_CS_PORT, DW_CS_PIN);
	//dw_sleep_usec(1);

	// send header
	for( i=0; i<headerLen; i++ )
	{
		tmp = dw_spi_transferbyte( header[i] );
		dw_spi_ledtoggle(5);
		// Debugging:
		//tmp = spi_send8(DW_SPI, header[i]);
	}
	// send body
	for( i=0; i<bodyLen; i++ )
	{
		tmp = dw_spi_transferbyte( body[i] );
		dw_spi_ledtoggle(5);
		// Debugging:
		//tmp = spi_send8(DW_SPI, body[i]);
	}

	// CS high and wait a bit
	GPIO_SetBits(DW_SPI_CS_PORT, DW_CS_PIN);
	//dw_sleep_usec(1);

	return 0;
}

/*
// Create a dummy function for a SPI write as well
int writetospi(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, const uint8_t* body)
{
	dw_spi_sendpacket(headerLen, &header, bodyLen, &body);
}
*/

int dw_spi_readpacket(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, uint8_t* body)
{
	int i;
	uint8_t tmp;


	// CS Low and wait a bit
	GPIO_ResetBits(DW_SPI_CS_PORT, DW_CS_PIN);

	//dw_sleep_usec(1);

	// send header
	for( i=0; i<headerLen; i++ )
	{
		tmp = dw_spi_transferbyte( header[i] );
		dw_spi_ledtoggle(5);
	}


	// send body
	for( i=0; i<bodyLen; i++ )
	{
		body[i] = dw_spi_transferbyte( 0x00 );
		dw_spi_ledtoggle(5);
	}

	// CS high and wait a bit
	GPIO_SetBits(DW_SPI_CS_PORT, DW_CS_PIN);
	//dw_sleep_usec(1);

	return 0;
}

// Create a dummy function for debugging in case a function with this name is called by DW library
/*
int readfromspi(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, uint8_t* body)
{
	dw_spi_readpacket(headerLen, &header, bodyLen, &body);
}
*/

void dw_sleep_usec(uint32_t usec)
{
	int i;
	for( i=0; i<usec*CLOCK_TICKS_PER_USEC; i++ )
	{
		__asm("nop");
	}
}

void dw_sleep_msec(uint32_t msec)
{
	dw_sleep_usec(msec*1000);
}


void dw_spi_configprescaler(uint16_t scalingfactor)
{
	SPI_InitTypeDef SPI_InitStructure;

	SPI_I2S_DeInit(DW_SPI);

	// SPI Mode setup
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;	 //
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = scalingfactor; //sets BR[2:0] bits - baudrate in SPI_CR1 reg bits 4-6
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(DW_SPI, &SPI_InitStructure);

	// Enable SPI
	SPI_Cmd(DW_SPI, ENABLE);
}
