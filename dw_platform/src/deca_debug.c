#include "deca_debug.h"
#include "deca_ranging.h"
#include "ntb_hwctrl.h"	// board specific DW SPI pins definitions
#include "stm32f30x.h"
//#include "stm32f4xx.h"
//#include "stm32f4_discovery.h"
#include "stm32f30x_spi.h"

void dw_debug_handler(uint8_t event)
{
	/*
	switch( event )
	{
		case DWRANGE_EVENT_ERROR:
			//ntb_led_error();
			break;

		case DWRANGE_EVENT_RXGOOD:
			//ntb_led_toggle(NTB_LED_ARM_RED);
			break;

		case DWRANGE_EVENT_TXGOOD:
			//ntb_led_toggle(NTB_LED_ARM_GREEN);
			break;

		case DWRANGE_EVENT_IRQ:
			//ntb_led_toggle(LED_ARM_GREEN);
			break;

		case DWRANGE_EVENT_RXINIT:
			//ntb_led_toggle(LED_ARM_RED);
			break;

		case DWRANGE_EVENT_RXFIN:
			break;

		case DWRANGE_EVENT_UNKNOWN_IRQ:
			break;

		case DWRANGE_EVENT_BADFRAME:
			break;

		default:
			break;
	}
	*/
}

