/**
  ******************************************************************************
  * @file    stm32f30x_fmc_sram.c
  * @author  MCD Application Team
  * @version V1.1.2
  * @date    14-August-2015
  * @brief   This file provides a set of functions needed to drive the 
  *          IS61WV51216BLL SRAM memory mounted on STM3210E-EVAL board.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f30x_fmc_sram.h"

/** @addtogroup STM32F30x_StdPeriph_Examples
  * @{
  */

/** @addtogroup FMC_SRAM
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define Bank1_SRAM3_ADDR    ((uint32_t)0x68000000)     
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void SRAM_GPIOConfig(void);
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configures the FMC and GPIOs to interface with the SRAM memory.
  *         This function must be called before any write/read operation
  *         on the SRAM.
  * @param  None 
  * @retval None
  */
void SRAM_Init(void)
{ 
  FMC_NORSRAMInitTypeDef  FMC_NORSRAMInitStructure;
  FMC_NORSRAMTimingInitTypeDef  NORSRAMTimingStructure;
  
  /* GPIO configuration for FMC SRAM bank */
  SRAM_GPIOConfig();
  
  /* Enable FMC clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FMC, ENABLE); 
  
/* FMC Configuration ---------------------------------------------------------*/
  /* SRAM Timing configuration */
  NORSRAMTimingStructure.FMC_AddressSetupTime = 2;
  NORSRAMTimingStructure.FMC_AddressHoldTime = 1;
  NORSRAMTimingStructure.FMC_DataSetupTime = 2;
  NORSRAMTimingStructure.FMC_BusTurnAroundDuration = 1;
  NORSRAMTimingStructure.FMC_CLKDivision = 2;
  NORSRAMTimingStructure.FMC_DataLatency = 2;
  NORSRAMTimingStructure.FMC_AccessMode = FMC_AccessMode_A; 

  /* FMC SRAM control configuration */
  FMC_NORSRAMInitStructure.FMC_Bank = FMC_Bank1_NORSRAM3;
  FMC_NORSRAMInitStructure.FMC_DataAddressMux = FMC_DataAddressMux_Disable;
  FMC_NORSRAMInitStructure.FMC_MemoryType = FMC_MemoryType_SRAM;
  FMC_NORSRAMInitStructure.FMC_MemoryDataWidth = SRAM_MEMORY_WIDTH;
  FMC_NORSRAMInitStructure.FMC_BurstAccessMode = SRAM_BURSTACCESS; 
  FMC_NORSRAMInitStructure.FMC_AsynchronousWait = FMC_AsynchronousWait_Disable;  
  FMC_NORSRAMInitStructure.FMC_WaitSignalPolarity = FMC_WaitSignalPolarity_Low;
  FMC_NORSRAMInitStructure.FMC_WrapMode = FMC_WrapMode_Disable;
  FMC_NORSRAMInitStructure.FMC_WaitSignalActive = FMC_WaitSignalActive_BeforeWaitState;
  FMC_NORSRAMInitStructure.FMC_WriteOperation = FMC_WriteOperation_Enable;
  FMC_NORSRAMInitStructure.FMC_WaitSignal = FMC_WaitSignal_Disable;
  FMC_NORSRAMInitStructure.FMC_ExtendedMode = FMC_ExtendedMode_Disable;
  FMC_NORSRAMInitStructure.FMC_WriteBurst = SRAM_WRITEBURST;
  FMC_NORSRAMInitStructure.FMC_ReadWriteTimingStruct = &NORSRAMTimingStructure;
  FMC_NORSRAMInitStructure.FMC_WriteTimingStruct = &NORSRAMTimingStructure;

  /* SRAM configuration */
  FMC_NORSRAMInit(&FMC_NORSRAMInitStructure); 

  /* Enable FMC Bank1_SRAM2 Bank */
  FMC_NORSRAMCmd(FMC_Bank1_NORSRAM3, ENABLE); 
}

/**
  * @brief  Writes a Half-word buffer to the FMC SRAM memory. 
  * @param  pBuffer : pointer to buffer. 
  * @param  WriteAddr : SRAM memory internal address from which the data will be 
  *         written.
  * @param  NumHalfwordToWrite : number of half-words to write. 
  * @retval None
  */
void SRAM_WriteBuffer(uint16_t* pBuffer, uint32_t WriteAddr, uint32_t NumHalfwordToWrite)
{
  for(; NumHalfwordToWrite != 0; NumHalfwordToWrite--) /*!< while there is data to write */
  {
    /*!< Transfer data to the memory */
    *(uint16_t *) (Bank1_SRAM3_ADDR + WriteAddr) = *pBuffer++;
    
    /*!< Increment the address*/  
    WriteAddr += 2;
  }   
}

/**
  * @brief  Reads a block of data from the FMC SRAM memory.
  * @param  pBuffer : pointer to the buffer that receives the data read from the 
  *         SRAM memory.
  * @param  ReadAddr : SRAM memory internal address to read from.
  * @param  NumHalfwordToRead : number of half-words to read.
  * @retval None
  */
void SRAM_ReadBuffer(uint16_t* pBuffer, uint32_t ReadAddr, uint32_t NumHalfwordToRead)
{
  for(; NumHalfwordToRead != 0; NumHalfwordToRead--) /*!< while there is data to read */
  {
    /*!< Read a half-word from the memory */
    *pBuffer++ = *(__IO uint16_t*) (Bank1_SRAM3_ADDR + ReadAddr);

    /*!< Increment the address*/  
    ReadAddr += 2;
  }  
}

/**
  * @brief  Configures all SRAM memory I/Os pins. 
  * @param  None. 
  * @retval None.
  */

static void SRAM_GPIOConfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  /* Enable GPIOs clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD | RCC_AHBPeriph_GPIOE | RCC_AHBPeriph_GPIOF |
                        RCC_AHBPeriph_GPIOG | RCC_AHBPeriph_GPIOH, ENABLE);
  
/*-- GPIOs Configuration -----------------------------------------------------*/
/*
 +-------------------+--------------------+--------------------+--------------------+
 +                       SRAM pins assignment                                       +                                
 +-------------------+--------------------+--------------------+--------------------+
 | PC6  <-> FMC_D2   | PE0  <-> FMC_NBL0  | PA3  <-> FMC_A0    | PH4 <-> FMC_A10    | 
 | PC5  <-> FMC_D1   | PE1  <-> FMC_NBL1  | PA4  <-> FMC_A1    | PH5 <-> FMC_A11    |
 | PC3  <-> FMC_A5   | PE2  <-> FMC_D12   | PA5  <-> FMC_A2    | PH2  <-> FMC_A8    |
 | PC4  <-> FMC_A6   | PE4  <-> FMC_A16   |                    | PH3  <-> FMC_A9    | 
 | PC1  <-> FMC_D9   | PE5  <-> FMC_D4    |                    | PH1  <-> FMC_A18   | 
 | PC2  <-> FMC_D10  |                    |                    |                    |
 |-------------------|--------------------|------------------- |--------------------|
 | PD5  <-> FMC_D3   | PG3 <-> FMC_A12    | PB4  <-> FMC_A4    | PF3  <-> FMC_A14   | 
 | PD4  <-> FMC_NOE  | PG4 <-> FMC_A13    | PB3  <-> FMC_A3    | PF2  <-> FMC_D13   |   
 | PD5  <-> FMC_NWE  | PG6 <-> FMC_D7     | PB6  <-> FMC_D0    | PF1  <-> FMC_D14   | 
 | PD4  <-> FMC_A7   | PG9 <-> FMC_NE2    | PB1  <-> FMC_D8    | PF5  <-> FMC_D5    |
 | PD7  <-> FMC_NE1  | PG1 <-> FMC_D15    |                    | PF4  <-> FMC_A15   |
 | PD3  <-> FMC_A17  |                    |                    | PF6  <-> FMC_D6    |
 | PD2  <-> FMC_D11  |                    |                    |                    |  
 |-------------------|--------------------|------------------- |--------------------|	                
					 
*/ 

  /* Common GPIO configuration */
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  /*############################### Data Bus #################################*/
  /* GPIOD configuration */
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_12);  
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_12); 
 
   /* GPIOD configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_8 | GPIO_Pin_9 |
                                GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* GPIOE configuration */  
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_12);  
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource12, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource15, GPIO_AF_12); 
  
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 |
                              GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                              GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  /*################################## Address Bus ###########################*/
  /* GPIOF configuration */ 
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource2, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource3, GPIO_AF_12);  
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource4, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource5, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource12, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource13, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource14, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource15, GPIO_AF_12);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3  | GPIO_Pin_4 | 
                                GPIO_Pin_5 | GPIO_Pin_12 | GPIO_Pin_13 |
                                GPIO_Pin_14| GPIO_Pin_15;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOH, GPIO_PinSource0, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOH, GPIO_PinSource1, GPIO_AF_12); 
  
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_Init(GPIOH, &GPIO_InitStructure);
  
  /* GPIOG configuration */ 
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource0, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource1, GPIO_AF_12);  
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource2, GPIO_AF_12); 
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource3, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource4, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource5, GPIO_AF_12);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |
                                GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
  
  /* GPIOD configuration */
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource11, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_12);  
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_12); 
  
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /*####################### NOE and NWE configuration ########################*/ 
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource4, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_12); 
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 |GPIO_Pin_5;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /*############################## NE3 configuration #########################*/
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource10, GPIO_AF_12);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
  
  /*####################### NBL0, NBL1 configuration #########################*/
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource0, GPIO_AF_12);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_12); 
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_Init(GPIOE, &GPIO_InitStructure); 
}

/**
  * @}
  */

/**
  * @}
  */  

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
