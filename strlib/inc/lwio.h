/**
  ******************************************************************************
  * @file    usbd_cdc_vcp.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-July-2011
  * @brief   Header for usbd_cdc_vcp.c file.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LWIO_H_
#define LWIO_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Defines
#define MAX_INT_BUFFER ( 32 )

int intlist2str(uint8_t* str, int* list, int len);
int uintlist2str(uint8_t* str, uint64_t* list, int len);
uint8_t* utoa(uint64_t num, uint8_t* str, int base);
int int2str(int num, uint8_t* str, int base);
void reverse(uint8_t str[], int length);


#endif /* LWIO_H_ */

