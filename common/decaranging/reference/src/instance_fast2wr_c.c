/*! ----------------------------------------------------------------------------
 *  @file    instance_fast2wr_c.c
 *  @brief   DecaWave application level Fast 2W ranging demo common instance functions
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
#include "compiler.h"
#include "port.h"
#include "deca_device_api.h"
#include "deca_spi.h"

#include "instance.h"

// -------------------------------------------------------------------------------------------------------------------


// -------------------------------------------------------------------------------------------------------------------
//      Data Definitions
// -------------------------------------------------------------------------------------------------------------------


// -------------------------------------------------------------------------------------------------------------------

//extern uint32 statushistory[5];
//extern void send_usbmessage(uint8 *string, int len);
//uint8 usbdata[80];
//int count=0;
// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------


// -------------------------------------------------------------------------------------------------------------------
#if NUM_INST != 1
#error These functions assume one instance only
#else

extern uint8 dwnsSFDlen[];

int instanceframeduration(int datalength) //duration in symbols
{
    int instance = 0 ;

	//configure the rx delay receive delay time, it is dependent on the message length
	float msgdatalen = 0;
	float preamblelen = 0;
	int sfdlen = 0;
	int x = 0;

	msgdatalen = (float) datalength;

	x = (int) ceil(msgdatalen*8/330.0f);

	msgdatalen = msgdatalen*8 + x*48;

	//assume PHR length is 172308us for 110k and 21539us for 850k/6.81M
	if(instance_data[instance].configData.dataRate == DWT_BR_110K)
	{
		msgdatalen *= 8205.13f;
		msgdatalen += 172308;

	}
	else if(instance_data[instance].configData.dataRate == DWT_BR_850K)
	{
		msgdatalen *= 1025.64f;
		msgdatalen += 21539;
	}
	else
	{
		msgdatalen *= 128.21f;
		msgdatalen += 21539;
	}

	//SFD length is 64 for 110k (always)
	//SFD length is 8 for 6.81M, and 16 for 850k, but can vary between 8 and 16 bytes
	sfdlen = dwnsSFDlen[instance_data[instance].configData.dataRate];

	switch (instance_data[instance].configData.txPreambLength)
    {
    case DWT_PLEN_4096 : preamblelen = 4096.0f; break;
    case DWT_PLEN_2048 : preamblelen = 2048.0f; break;
    case DWT_PLEN_1536 : preamblelen = 1536.0f; break;
    case DWT_PLEN_1024 : preamblelen = 1024.0f; break;
    case DWT_PLEN_512  : preamblelen = 512.0f; break;
    case DWT_PLEN_256  : preamblelen = 256.0f; break;
    case DWT_PLEN_128  : preamblelen = 128.0f; break;
    case DWT_PLEN_64   : preamblelen = 64.0f; break;
    }

	//preamble  = plen * (994 or 1018) depending on 16 or 64 PRF
	if(instance_data[instance].configData.prf == DWT_PRF_16M)
	{
		preamblelen = (sfdlen + preamblelen) * 0.99359f;
	}
	else
	{
		preamblelen = (sfdlen + preamblelen) * 1.01763f;
	}


	//total time the frame takes in symbols
	return (16 + (int)((preamblelen + (msgdatalen/1000.0))/ 1.0256));

}

// -------------------------------------------------------------------------------------------------------------------
// function to initialise instance structures
//
// Returns 0 on success and -1 on error
int instance_init_f(int mode)
{
    int instance = 0 ;

    instance_data[instance].mode =  mode;                                // assume anchor,
    instance_data[instance].testAppState = TA_INIT ;

    // if using auto CRC check (DWT_INT_RFCG and DWT_INT_RFCE) are used instead of DWT_INT_RDFR flag
    // other errors which need to be checked (as they disable receiver) are
    //dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);

    //using auto rx re-enable so don't need to trigger on error frames - see inst->rxautoreenable it is set to 1
    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | (DWT_INT_SFDT | DWT_INT_RFTO /*| DWT_INT_RXPTO*/), 1);

    //this is platform dependent - only program if DW EVK/EVB
    dwt_setleds(3) ; //configure the GPIOs which control the LEDs on EVBs

    dwt_setcallbacks(instance_txcallback, instance_rxcallback);

    if(mode == ANCHOR)
    	instance_setapprun(testapprun_af);
    else
    	instance_setapprun(testapprun_tf);

    return 0 ;
}


//configure the response times and rx turn on delays
void instance_config_f(void)
{
	int instance = 0;
	int rngInitDly = FIXED_DP_REPLY_DELAY_US;
	int polRespDly = FIXED_RP_REPLY_DELAY_US;

	if(instance_data[instance].configData.txPreambLength == DWT_PLEN_512)
	{
		rngInitDly += 400;
		polRespDly += 400;
	}


	instance_data[instance].fwtoTimeB_sy = instanceframeduration(RANGINGINIT_MSG_LEN + FRAME_CRTL_AND_ADDRESS_LS + FRAME_CRC);

	instance_data[instance].fwtoTime_sy = instanceframeduration(TAG_FINAL_F_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC);


	instance_data[instance].fixedRngInitReplyDelay32h = (uint32) ((convertmicrosectodevicetimeu32 (rngInitDly) & MASK_TXDTS) >> 8);
	instance_data[instance].fixedFastReplyDelay32h = (uint32) ((convertmicrosectodevicetimeu32 (polRespDly) & MASK_TXDTS) >> 8);

	//response will be sent after FIXED_DP_REPLY_DELAY_US us (thus delay the receiver turn on by FIXED_DP_REPLY_DELAY_US - frame duration = 390sym ~ 399us)
	//use delayed rx on (wait4resp timer) - this value is applied when the TX frame is done/sent, so this value can be written after TX is started
	instance_data[instance].rnginitW4Rdelay_sy = (rngInitDly - instance_data[instance].fwtoTimeB_sy);  //units are ~us - wait for wait4respTIM before RX on (delay RX)
	instance_data[instance].fixedReplyDelay_sy = (polRespDly - instance_data[instance].fwtoTime_sy);  //units are ~us - wait for wait4respTIM before RX on (delay RX)
}

#endif


/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
