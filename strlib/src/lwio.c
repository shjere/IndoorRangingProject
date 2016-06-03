/**
 ******************************************************************************
 * @file    usbd_cdc_vcp.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    22-July-2011
 * @brief   Generic media access Layer.
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

/* Includes ------------------------------------------------------------------*/
#include "lwio.h"

// converts list to CSV string, returns number of bytes written
int intlist2str(uint8_t* str, int* list, int len)
{
  int i, j;
  int msglen = 0;
  uint8_t intbuf[MAX_INT_BUFFER];

  for( i=0; i<len; i++)
  {
    // convert integer to string
    int intlen = int2str(list[i], intbuf, 10);
    // copy int ascii bytes to return string
    for(j = 0; j < intlen; j++)
    {
      str[msglen++] = intbuf[j];
    }

    // if it's not the final int, add a comma. else add a \n
    if( i < (len-1) )
    {
      str[msglen++] = ',';
    }
    else
    {
      str[msglen++] = '\n';
    }

  }

  return msglen;
}

// converts uint64_t list to CSV string, returns number of bytes written
int uintlist2str(uint8_t* str, uint64_t* list, int len)
{
  int i, j;
  int msglen = 0;
  uint8_t uintbuf[MAX_INT_BUFFER];

  for( i=0; i<len; i++)
  {
    // convert integer to string
    int uintlen = uint2str(list[i], uintbuf, 10);
    // copy int ascii bytes to return string
    for(j = 0; j < uintlen; j++)
    {
      str[msglen++] = uintbuf[j];
    }

    // if it's not the final int, add a comma. else add a \n
    if( i < (len-1) )
    {
      str[msglen++] = ',';
    }
    else
    {
      str[msglen++] = '\n';
    }

  }

  return msglen;
}


// Unsigned 64-bit version of itoa
int uint2str(uint64_t num, uint8_t* str, int base)
{
    int i = 0;
 
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }
 
    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    // Reverse the string
    reverse(str, i);
 
    str[i] = '\0'; // Append string terminator
 
    return i;
}

// integer to string conversion
int int2str(int num, uint8_t* str, int base)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }
 
    // In standard itoa(), negative numbers are handled only with 
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0 && base == 10)
    {
        isNegative = true;
        num = -num;
    }
 
    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
  
    // Reverse the string
    reverse(str, i);

    str[i] = '\0'; // Append string terminator
 
    return i;
}

/* A utility function to reverse a string  */
void reverse(uint8_t str[], int length)
{
    int start = 0;
    int end = length -1;
    uint8_t tmp;
    while (start < end)
    {
        tmp = *(str+end);
        *(str+end) = *(str+start);
        *(str+start) = tmp; 
        start++;
        end--;
    }
}