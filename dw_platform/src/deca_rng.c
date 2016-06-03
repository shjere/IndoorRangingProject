#include <stdint.h>
//#include "stm32f4xx.h"
#include "stm32f30x.h"
//#include "stm32f4_discovery.h"
//#include "stm32f4xx_rng.h"
#include "ntb_hwctrl.h"

#define MAX_32BIT_INT (4294967296)

void dw_rng_init()
{
	//RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	//RNG_Cmd(ENABLE);
}


float dw_rng_float()
{
	uint32_t rand32;
	//while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET){}
	//rand32 = RNG_GetRandomNumber();
	//return (float)rand32/MAX_32BIT_INT;
	return (float)MAX_32BIT_INT;
}
