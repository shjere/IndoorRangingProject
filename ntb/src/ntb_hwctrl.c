#include "ntb_hwctrl.h"
//#include "stm32f30x_spi.h"
//#include <stm32f30x_gpio.h>
//#include <stm32f30x_rcc.h>
//#include <stm32f30x_i2c.h>



/* Function definitions */
void ntb_init_hw()
{
  //ntb_init_uid();
  //ntb_init_gpio();  
  //ntb_deinit_spiexp();
  ntb_init_gpio();
  ntb_init_spidw();
  dw_init_irq();
}

/* Function to enable GPIO ports for DW_SPI and LED  */
void ntb_init_gpio()
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  // Enable Port A clock for SPI1 pins
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

  // Enable Port B --> port on which the LED is
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

  //  Configure pins used by SPI1:
  //    PA5 = SCLK
  //    PA6 = MISO
  //    PA7 = MOSI
  //
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  //Connect SPI1 pins to SPI alternate function.
  //
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_5);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_5);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_5);

  //  Configure the Chip Select for SPI1:
  //    PA8 - SPI1 chip select pin.
  //    
  //
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);  

  //  Set PA8 high as we will be using active low for the
  //  device select.
  //
  
  GPIO_SetBits(GPIOA, GPIO_Pin_8);

  // Configure LED on PB4 in output pushpull mode
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);

}

void led_on()
{
  GPIO_SetBits(GPIOB, GPIO_Pin_4);
}

void led_off()
{
  GPIO_ResetBits(GPIOB, GPIO_Pin_4);
}

void led_toggle_fast()
{
  while(1)
  {
    GPIO_SetBits(GPIOB, GPIO_Pin_4);

    int counter=0.45e6;
  
    while(counter--)
    {
      __asm("nop");
    }

    GPIO_ResetBits(GPIOB, GPIO_Pin_4);
    
    int counter2=0.45e6;

    while(counter2--)
    {
      __asm("nop");
    }
  }
}

void led_toggle_medium()
{
  while(1)
  {
    GPIO_SetBits(GPIOB, GPIO_Pin_4);

    int counter=1e6;
  
    while(counter--)
    {
      __asm("nop");
    }

    GPIO_ResetBits(GPIOB, GPIO_Pin_4);
    
    int counter2=1e6;

    while(counter2--)
    {
      __asm("nop");
    }
  }
}

void led_toggle_slow()
{
  GPIO_SetBits(GPIOB, GPIO_Pin_4);

    int counter=2.5e6;
  
    while(counter--)
    {
      __asm("nop");
    }

    GPIO_ResetBits(GPIOB, GPIO_Pin_4);
    
    int counter2=2.5e6;

    while(counter2--)
    {
      __asm("nop");
    }
}

void dw_spi_ledtoggle(int ms)
{
  led_on();
  ntb_busywait_ms(ms);
  led_off();
  ntb_busywait_ms(ms);
}


// This function configures the RST and IRQ pins for the DW

void dw_init_irq()
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  EXTI_InitTypeDef EXTI_InitStruct;
  NVIC_InitTypeDef NVIC_InitStruct;

  // Enable clock for port B
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

  // Configure the DW RSTn pin : PB8
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP; 
  GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_SetBits(GPIOB, GPIO_Pin_8);

  // Configure the DW IRQ pin : PB9 
  GPIO_InitStruct.GPIO_Pin = DW_IRQ_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DW_IRQ_PORT, &GPIO_InitStruct);
  SYSCFG_EXTILineConfig(DW_IRQ_EXTI, DW_IRQ_PSOURCE);

  // Configure the EXTI for DW IRQ
  EXTI_InitStruct.EXTI_Line = DW_IRQ_LINE;
  EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStruct);

  NVIC_InitStruct.NVIC_IRQChannel = DW_IRQ_CHNL;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStruct);
}


void ntb_init_spidw()
{
  // The DecaWave uses the SPI1 peripheral on the STM32F3 MCU to communicate with it

  
  SPI_InitTypeDef SPI_InitStruct;
  

  SPI_I2S_DeInit(SPI1);

  /* ------------ Fire up SPI --------------- */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  /* configure SPI1 in Mode 0 
   * CPOL = 0 --> clock is low when idle
   * CPHA = 0 --> data is sampled at the first edge
   */
  SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStruct.SPI_Mode = SPI_Mode_Master;  
  SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b; 
  SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;       
  SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128; // 4-64
  SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStruct.SPI_CRCPolynomial = 7;

  SPI_Init(SPI1, &SPI_InitStruct); 

  // Configure the data size for SPI transfers
  SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

  /* Initialize the FIFO threshold */
  SPI_RxFIFOThresholdConfig(SPI1, SPI_RxFIFOThreshold_QF);
  

  // Disable SPI SS output
  SPI_SSOutputCmd(SPI1, DISABLE);


  // Enable SPI
  SPI_Cmd(SPI1, ENABLE);

 

  

  // ---------- Turn off orange LED to say we're done -------------
  //dw_sleep_msec(500);
}



// LED functions




void ntb_soft_reset()
{
	NVIC_SystemReset();
}

//Quick hack, approximately 1ms delay
void ntb_busywait_ms(int ms)
{
   while (ms-- > 0) {
      volatile int x=5971;
      while (x-- > 0)
         __asm("nop");
   }
}
