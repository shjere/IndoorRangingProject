// Includes
#include "deca_ranging.h"
#include <string.h>
#include <math.h>

// state machine variables
uint8_t  current_state = DWR_STATE_IDLE;
uint8_t  current_rangeSeq  = 0;
uint8_t  current_beaconSeq = 0;
uint64_t time_tx_init;
uint64_t time_rx_init;
uint64_t time_tx_resp;
uint64_t time_rx_resp;
uint64_t time_tx_fin;
uint64_t time_rx_fin;
uint64_t time_tx_bcn;
uint32_t time_slotdelay;

// DW1000 Timer overflow accounting
volatile uint64_t time_last = 0;
volatile uint32_t timer_overflows = 0;

// RX / TX data storage and framing
uint8_t rx_buffer[STANDARD_FRAME_SIZE];
DWR_MsgData_t msgToSend;
ieee_frame_dsss_t frameToSend;

// User configuration
DWR_Config_t user_config;

// has the decawave been initialized?
int dwr_initialized = 0;

// channel configuration (see DW1000 user manual)
dwt_config_t ch_config = {
	.chan = 2,
	.rxCode = 9,
	.txCode = 9,
	.prf = DWT_PRF_64M,
	.dataRate = DWT_BR_6M8,
	.txPreambLength = DWT_PLEN_128,
	.rxPAC = DWT_PAC8,
	.nsSFD = 0,
	.phrMode = DWT_PHRMODE_STD,
	.sfdTO = (256+64-32),
	.smartPowerEn = 0
};

// tx power configuration (see DW1000 user manual)
dwt_txconfig_t tx_calib = {
	.PGdly = 0xC2,       // ch2
	.power = 0x00000000
};


// ===== Private Transmit Functions =====
static void dw_send_range_resp(uint8_t* destAddr, uint16_t seqNum, uint32_t txtime);
static void dw_send_range_fin(uint8_t* destAddr, uint16_t seqNum, uint64_t rxtime, uint32_t txtime);
static void dw_send_range_sum(uint16_t seqNum, uint16_t msgSrc, uint64_t t1, uint64_t t2, uint64_t t3, uint64_t t4, uint64_t t5, uint64_t t6, float fploss );
static void dw_send_ieee_broadcast(DWR_MsgData_t *msg, uint32_t txtime);
static void dw_send_ieee_data_delayed(uint8_t* destAddr, DWR_MsgData_t *msg, uint32_t txtime);

// ===== Private Chnl Meas. Functions =====
static void dw_getRxQuality(float *fppwr, float *cirp, float *fploss);

// ===== Private Debugging Functions =====
static void dw_debug(DWR_EventType et);

// ===== Timer overflow functions ======
static void dw_process_time(uint64_t rawtime);
static uint64_t dw_project_time(uint64_t rawtime);


// ========== Ranging Engine Initialization ==========
int decaranging_init( DWR_Config_t config )	
{
	// initialization result
	int result;

	// copy user configuration
	user_config = config;

	// initialize TX frame for 802.15.4 Data
	frameToSend.frameCtrl[0] = 0x41;
	frameToSend.frameCtrl[1] = 0x88;
	frameToSend.seqNum = 0;
	frameToSend.panId[0] = user_config.panId & 0xFF;
	frameToSend.panId[1] = (user_config.panId >> 8) & 0xFF;
	frameToSend.sourceAddr[0] = user_config.addr & 0xFF;
	frameToSend.sourceAddr[1] = (user_config.addr >> 8) & 0xFF;

	// initialize tx slot delay based on short address
	time_slotdelay = (uint32_t)(frameToSend.sourceAddr[0]*DWR_REPLY_SLOTPERIOD_US*1e3/DWR_TIMER_UNITS_HI_NS);

	// wire up the SPI and sleep functions to the DW driver
	dwt_wire_spifcns(user_config.spiSendPacket, user_config.spiReadPacket);
	dwt_wire_sleepfcns(user_config.sleepms);

	// reset the DW1000
	dwt_softreset();

	// One-time-programmable (OTP) memory loading
	int otp_options = DWT_LOADUCODE | DWT_LOADLDO | DWT_LOADXTALTRIM;

	// load from OTP and initialize DW1000
	result = dwt_initialise(otp_options);
	if( result != DWT_SUCCESS ) return DWR_RETURN_ERR;

	// configure the channel
	dwt_configure(&ch_config, otp_options);
	dwt_configuretxrf(&tx_calib);
	
    // antenna delay calibration
    dwt_setrxantennadelay(DWM1000_ANT_DELAY);
    dwt_settxantennadelay(DWM1000_ANT_DELAY);


    // configure RX mode and timeouts
    dwt_setrxmode(DWT_RX_NORMAL, 0, 0);
    dwt_setautorxreenable(1);
    if( user_config.nodeOpts & DWR_OPT_RXTO )
    {
    	dwt_setrxtimeout(DWR_RX_TIMEOUT_US);
    }
    else
    {
		dwt_setrxtimeout(0);
	}

	// only allow 802.15.4 data frames
	dwt_enableframefilter(DWT_FF_DATA_EN);

	// only allow interrupts on good rx frames and good tx frames
	dwt_setinterrupt( DWT_INT_RFCG | DWT_INT_TFRS, 1 );

	// set PAN and short address
	dwt_setpanid( user_config.panId );
	dwt_setaddress16( user_config.addr );

	// start listening on appropriate node types
	if( !(user_config.nodeOpts & DWR_OPT_RXNONE) )
	{
		dwt_rxenable(0);
	}
	
	// start in the idle state
	current_state = DWR_STATE_IDLE;

	// initialize the red and green LEDs as GPIO (HW-Specific)
	//dwt_ntbled_init();

	// and that's it! We're done initializing
	dwr_initialized = 1;

	return DWR_RETURN_OK;
}


// read the device ID from the DW1000
uint32_t decaranging_devid()
{
	return dwt_readdevid();
}

// handle TX and RX good events, sending responses or notifying user where appropriate
void decaranging_isr()
{	
	// If we're not initialized, don't do anything
	if( !dwr_initialized )
		return;

	// Let user know an IRQ was triggered
	dw_debug(DWR_EVENT_IRQ);

	// Read the event status
	uint32_t status = dwt_read32bitreg(SYS_STATUS_ID);
	uint32_t bitsToClear = 0x00;

	if(status & SYS_STATUS_RXFCG) // Receiver FCS Good
	{
		// let the user know a good frame was received
		dw_debug( DWR_EVENT_RXGOOD );

		bitsToClear |= status & CLEAR_ALLRXGOOD_EVENTS;
		dwt_write32bitreg(SYS_STATUS_ID, bitsToClear);

		// bug 634 - overrun overwrites the frame info data... so both frames should be discarded
		// read frame info and other registers and check for overflow again
		// if overflow set then discard both frames... 
		if (status & SYS_STATUS_RXOVRR) 
		{ 
			//when the overrun happens the frame info data of the buffer A (which contains the older frame e.g. seq. num = x) 
			//will be corrupted with the latest frame (seq. num = x + 2) data, both the host and IC are pointing to buffer A
			//we are going to discard this frame - turn off transceiver and reset receiver
			current_state = DWR_STATE_IDLE;
			dwt_forcetrxoff();
			dwt_rxreset();
			dwt_rxenable(0);
		}
		else //no overrun condition - proceed to process the frame
		{
			// clear all receive status bits
			bitsToClear |= status & CLEAR_ALLRXGOOD_EVENTS;
			dwt_write32bitreg(SYS_STATUS_ID, bitsToClear);

			// length of received data
			uint16_t frameLen = dwt_read16bitoffsetreg(RX_FINFO_ID, 0) & 0x3FF;

			// read entire frame into buffer
			dwt_readfromdevice(RX_BUFFER_ID, 0, frameLen, rx_buffer);

			// get frame type (bits 0-2 of frame)
			uint8_t frameType = rx_buffer[0] & 0x03;

			// ---------- HANDLE DATA FRAMES -------------
			if(frameType == FRAME_DATA)
			{
				// read rx timestamp
				uint8_t rx_time_bytes[5];
				dwt_readrxtimestamp(rx_time_bytes);
				uint64_t rx_time = ((uint64_t)rx_time_bytes[4] << 32) + ((uint64_t)rx_time_bytes[3] << 24) + ((uint64_t)rx_time_bytes[2] << 16) + 
								   ((uint64_t)rx_time_bytes[1] <<  8) + ((uint64_t)rx_time_bytes[0]);

				// process time for overflows
				dw_process_time( rx_time );

				// extract frame data
				ieee_frame_dsss_t *frame = (ieee_frame_dsss_t*)(rx_buffer);

				// extract payload
				DWR_MsgData_t *msg = (DWR_MsgData_t*)(frame->msgData);

				switch( msg->msgType )
				{
					case DWR_MSG_RANGE_INIT:
						// some node wants range information to nearby nodes. We should
						// respond if we have RSPTWR set. 

						// *Note: We'll start a range sequence from ANY state, so we don't depend on it having
						// returned to the INIT state. This helps recover from bad states.
						if( user_config.nodeOpts & DWR_OPT_RSPTWR )
						{
							// update range state variable and times
							current_state = DWR_STATE_RANGE_INIT;
							// update new range sequence id
							//current_rangeSeq = msg->rangeSeq;
							// update rx time of the init message
							time_rx_init = dw_project_time( rx_time );
							// extract tx time of the init message
							time_tx_init = msg->t_tx;
							// schedule a delayed response with random dithering
							uint32_t tx_time_min = (uint32_t)(rx_time>>8) + ( DWR_REPLY_DELAY_US )*1e3/DWR_TIMER_UNITS_HI_NS;
							// add slot delay and mask top 31 bits
							uint32_t tx_time = (tx_time_min + time_slotdelay) & DWR_TIMER_HI_31_BITS;
							// schedule the (delayed) response - takes high 32 bits of 40 bit timer
							dw_send_range_resp(frame->sourceAddr, msg->rangeSeq, tx_time);
							// update the tx time of the response message (expand hi 32 bits to 40 bit timer)
							time_tx_resp = dw_project_time( ((uint64_t)tx_time)<<8 );
							// update new range state
							current_state = DWR_STATE_RANGE_RESP;
							// send debug command
							dw_debug(DWR_EVENT_RXINIT);
						}
						break;
					case DWR_MSG_RANGE_RESP:
						// A node we sent a range init to has responded. Store the rx timestamp and send
						// a reply to that same node. First, check to make sure this data is for the correct
						// range sequence. And that we were in the range init state prior to this
						/*
						if( current_rangeSeq != msg->rangeSeq || current_state != DWR_STATE_RANGE_INIT )
						{
							// bad packet and/or collision. reset transceiver and state machine.
							dwt_forcetrxoff();
							dwt_rxenable(0);
							current_state = DWR_STATE_IDLE;
							break;
						}
						*/
						// Otherwise, the range sequence is correct and the state transition was correct.
						// Store the time we received this message
						time_rx_resp = dw_project_time( rx_time );
						// no dithering required here for response time. Do it as fast as possible
						uint32_t tx_time = (uint32_t)(rx_time>>8) + ( DWR_REPLY_DELAY_US )*1e3/DWR_TIMER_UNITS_HI_NS;
						tx_time &= DWR_TIMER_HI_31_BITS;
						// schedule the (delayed) response - takes high 32 bits of 40 bit timer
						dw_send_range_fin(frame->sourceAddr, msg->rangeSeq, time_rx_resp, tx_time);
						// Note: we do not update the new range state to "FINAL" because we may still be waiting
						// on additional range response packets from other nodes.
						break;
					case DWR_MSG_RANGE_FINAL:
						// A node that we've been ranging with has sent the final ranging information required to
						// calculate a range estimate. First let's make sure the range sequence and range state are
						// correct.
						/*
						if( current_rangeSeq != msg->rangeSeq || current_state != DWR_STATE_RANGE_RESP )
						{
							// bad packet and/or collision. reset transceiver and state machine.
							current_state = DWR_STATE_IDLE;
							dwt_forcetrxoff();
							dwt_rxenable(0);
							break;
						}
						*/

						// Otherwise, extract the final timing information and calculate range.
						time_rx_fin  = dw_project_time( rx_time );;
						time_tx_fin  = msg->t_tx;
						time_rx_resp = msg->t_rx;

						// And now we have enough information to estimate range and inform the host application
						/*
						float tround1_us = (time_rx_resp - time_tx_init)*0.00001565;
						float tround2_us = (time_rx_fin  - time_tx_resp)*0.00001565;
						float treply1_us = (time_tx_resp - time_rx_init)*0.00001565;
						float treply2_us = (time_tx_fin  - time_rx_resp)*0.00001565;

						// calculate time of flight in microseconds
						float ToF_us = (tround1_us*tround2_us - treply1_us*treply2_us)/(tround1_us + tround2_us + treply1_us + treply2_us);

						// get first path loss coefficient
						float fppwr, cirp, fploss;
						dw_getRxQuality(&fppwr, &cirp, &fploss);

						DWR_RangeEst_t range_est = {
							.nodeOpts = msg->nodeOpts,
							.nodeAddr = frame->sourceAddr[0] + (frame->sourceAddr[1] << 8),
							.rangeEst = ( ToF_us*SPEED_OF_LIGHT_CM_PER_US )
						};
						*/

						float fppwr, cirp, fploss;
						dw_getRxQuality(&fppwr, &cirp, &fploss);

						DWR_TwrTiming_t timing_info = 
						{
							.seq = msg->rangeSeq,
							.srcAddr = frame->sourceAddr[0] + (frame->sourceAddr[1] << 8),
							.dstAddr = user_config.addr,
							.tstamp1 = time_tx_init,
							.tstamp2 = time_rx_init,
							.tstamp3 = time_tx_resp,
							.tstamp4 = time_rx_resp,
							.tstamp5 = time_tx_fin,
							.tstamp6 = time_rx_fin,
							.fppwr = fppwr,
							.cirp = cirp,
							.fploss = fploss
						};

						// range complete debug message
						dw_debug(DWR_EVENT_RXFIN);

						// If this node has no gateway, relay the summary
						if( user_config.nodeOpts & DWR_OPT_SENDSUM )
						{
							dw_send_range_sum(timing_info.seq, timing_info.srcAddr, time_tx_init, time_rx_init, time_tx_resp, time_rx_resp, time_tx_fin, time_rx_fin, fploss);
						}
						user_config.cbRangeComplete(&timing_info);

						// range sequence complete, back to idle mode
						current_state = DWR_STATE_IDLE;

						// reset the receiver
						dwt_rxenable(0);

						break;
					case DWR_MSG_RANGE_SUMMARY:
						// if this node is not set up to relay messages, turn receiver back on and quit
						if( !(user_config.nodeOpts & DWR_OPT_GATEWAY) )
						{
							// reset the receiver and quit ISR
							dwt_rxenable(0);
							break;
						}

						// package up the timing info from the message
						DWR_TwrTiming_t relay_info = 
						{
							.seq = msg->rangeSeq,
							.srcAddr = msg->msgSrcAddr[0] + (msg->msgSrcAddr[1] << 8),
							.dstAddr = frame->sourceAddr[0] + (frame->sourceAddr[1] << 8),
							.fppwr  = 0,
							.cirp   = 0,
							.fploss = ((float)msg->fploss)/-10.0
						};

						relay_info.tstamp1 = msg->t_tx;
						relay_info.tstamp2 = msg->t_rx;
						relay_info.tstamp3 = msg->ts3;
						relay_info.tstamp4 = msg->ts4;
						relay_info.tstamp5 = msg->ts5;
						relay_info.tstamp6 = msg->ts6;

						user_config.cbRangeComplete(&relay_info);

						dw_debug(DWR_EVENT_RXSUM);

						// reset the receiver
						dwt_rxenable(0);

						break;
					case DWR_MSG_BEACON:
						// for beacon messages, we should relay the RX time to the user if we're the correct node type
						if( user_config.nodeOpts & DWR_OPT_RSPBCN )
						{
							// extract the transmitted time
							time_tx_bcn = msg->t_tx;

							// get rx quality stats
							float fppwr, cirp, fploss;
							dw_getRxQuality(&fppwr, &cirp, &fploss);

							// create beacon struct
							DWR_Beacon_t beacon = {
								.nodeAddr = frame->sourceAddr[0] + (frame->sourceAddr[1]<<8),
								.seq = msg->rangeSeq,
								.txstamp = time_tx_bcn,
								.rxstamp = rx_time,
								.fppwr = fppwr,
								.cirp = cirp,
								.fploss = fploss
							};
							// call user-provided callback with the DW1000's rx timestamp
							user_config.cbBeaconReceived(&beacon);
						}
						// reset the receiver
						dwt_rxenable(0);
						break;
					default:
						// We'll only get here if someone's sending 802.15.4 broadcast frames and not following our protocol
						current_state = DWR_STATE_IDLE;
						dwt_rxenable(0);
						break;
				}
			}

			// ---------- HANDLE OTHER ------------
			else
			{
				// we don't handle any other frame types, but if we happened to get here turn rx back on.
				current_state = DWR_STATE_IDLE;
				dwt_rxenable(0);
			}

		}//end of no overrun 

	} // end if CRC is good
	else
	//
	// Check for TX frame sent event and signal to upper layer.
	//
	if (status & SYS_STATUS_TXFRS)  // Transmit Frame Sent
	{
		bitsToClear |= CLEAR_ALLTX_EVENTS;
		dwt_write32bitreg(SYS_STATUS_ID, bitsToClear) ;       

		// let the user know the transmission was successful
		dw_debug(DWR_EVENT_TXGOOD);
	}
	else if (status & SYS_STATUS_RXRFTO) // Receiver Frame Wait timeout:
	{
		bitsToClear |= status & SYS_STATUS_RXRFTO;
		dwt_write32bitreg(SYS_STATUS_ID, bitsToClear);

		// force into idle
		dwt_forcetrxoff();
		// listen again
		dwt_rxenable(0);       
	}
	else // (status & CLEAR_ALLRXERROR_EVENTS) //catches all other error events 
	{
		bitsToClear |= status & CLEAR_ALLRXERROR_EVENTS;
		dwt_write32bitreg(SYS_STATUS_ID, bitsToClear);    

		dwt_forcetrxoff(); //this will clear all events
		dwt_rxreset();	//reset the RX

		dwt_rxenable(0);

		status &= CLEAR_ALLTX_EVENTS;
	}
}

// send beacon
int decaranging_sendbcn()
{
	if( !dwr_initialized )
		return DWR_RETURN_ERR;

	/*

	// ensure trx is in idle state
	dwt_forcetrxoff();

	// calculate time to send broadcast in the future
	uint32_t time_now = dwt_readsystimestamphi32();
	uint32_t txtime = time_now + ( DWR_REPLY_DELAY_US )*1e3/DWR_TIMER_UNITS_HI_NS;

	// mask the top 31 bits of txtime
	txtime &= DWR_TIMER_HI_31_BITS;

	// create the beacon message
	msgToSend.msgType  = DWR_MSG_BEACON;
	msgToSend.rangeSeq = current_beaconSeq++;
	msgToSend.t_tx[0] = 0x00;
	msgToSend.t_tx[1] = (uint8_t)(txtime);
	msgToSend.t_tx[2] = (uint8_t)(txtime>>8);
	msgToSend.t_tx[3] = (uint8_t)(txtime>>16);
	msgToSend.t_tx[4] = (uint8_t)(txtime>>24);

	// send broadcast data frame
	dw_send_ieee_broadcast(&msgToSend, txtime);
	// listen if this node is RXALL or RXTO (listen to all or listen and timeout)
	if( user_config.nodeOpts & DWR_OPT_RXALL || user_config.nodeOpts & DWR_OPT_RXTO )
		dwt_rxenable(0);
	*/

	return DWR_RETURN_OK;
}




// send range init for two-way ranging
int decaranging_sendtwr()
{
	if( !dwr_initialized )
		return DWR_RETURN_ERR;

	// ensure trx is in idle state
	dwt_forcetrxoff();

	// calculate time to send broadcast in the future
	uint32_t time_now = dwt_readsystimestamphi32();
	uint32_t txtime = time_now + ( DWR_REPLY_DELAY_US )*1e3/DWR_TIMER_UNITS_HI_NS;
	dw_process_time( ((uint64_t)time_now)<<8 );

	// mask the top 31 bits of txtime
	txtime &= DWR_TIMER_HI_31_BITS;

	// create probe message with new random range sequence
	msgToSend.msgType  = DWR_MSG_RANGE_INIT;
	msgToSend.rangeSeq = current_rangeSeq++;
	msgToSend.t_tx = dw_project_time( ((uint64_t)txtime)<<8 );

	// send broadcast data frame
	dw_send_ieee_broadcast(&msgToSend, txtime);

	// listen if this node is RXALL or RXTO (listen to all or listen and timeout)
	if( user_config.nodeOpts & DWR_OPT_RXALL || user_config.nodeOpts & DWR_OPT_RXTO )
		dwt_rxenable(0);

	// update our state
	current_state = DWR_STATE_RANGE_INIT;

	return DWR_RETURN_OK;
}

static void dw_send_range_resp(uint8_t* destAddr, uint16_t seqNum, uint32_t txtime)
{
	// create range response message
	msgToSend.msgType  = DWR_MSG_RANGE_RESP;
	msgToSend.rangeSeq = seqNum;

	// send delayed data frame
	dw_send_ieee_data_delayed(destAddr, &msgToSend, txtime);
	// listen immediately afterwards
	dwt_rxenable(0);
}

static void dw_send_range_fin(uint8_t* destAddr, uint16_t seqNum, uint64_t rxtime, uint32_t txtime)
{
	// create range response message
	msgToSend.msgType  = DWR_MSG_RANGE_FINAL;
	msgToSend.rangeSeq = seqNum;
	msgToSend.t_tx = dw_project_time( ((uint64_t)txtime)<<8 );
	msgToSend.t_rx = rxtime;
	
	// send delayed data frame
	dw_send_ieee_data_delayed(destAddr, &msgToSend, txtime);
	// only listen after the range_fin if we're told to always listen
	if( user_config.nodeOpts & DWR_OPT_RXALL )
		dwt_rxenable(0);
}

static void dw_send_range_sum(uint16_t seqNum, uint16_t msgSrc, uint64_t t1, uint64_t t2, uint64_t t3, uint64_t t4, uint64_t t5, uint64_t t6, float fploss )
{
	// calculate time to send broadcast in the future
	uint32_t time_now = dwt_readsystimestamphi32();
	uint32_t txtime = time_now + ( DWR_REPLY_DELAY_US )*1e3/DWR_TIMER_UNITS_HI_NS;
	txtime &= DWR_TIMER_HI_31_BITS;

	// create range summary message
	msgToSend.msgType  = DWR_MSG_RANGE_SUMMARY;
	msgToSend.rangeSeq = seqNum;

	// message source address
	msgToSend.msgSrcAddr[0] = (uint8_t)(msgSrc);
	msgToSend.msgSrcAddr[1] = (uint8_t)(msgSrc>>8);

	// copy timestamps over
	msgToSend.t_tx = t1;
	msgToSend.t_rx = t2;
	msgToSend.ts3 = t3;
	msgToSend.ts4 = t4;
	msgToSend.ts5 = t5;
	msgToSend.ts6 = t6;

	// path loss
	msgToSend.fploss = (uint8_t)(fploss*-10);

	// send broadcast data frame
	dw_send_ieee_broadcast(&msgToSend, txtime);

	// only listen after the range_fin if we're told to always listen
	if( user_config.nodeOpts & DWR_OPT_RXALL )
		dwt_rxenable(0);
}

static void dw_send_ieee_broadcast(DWR_MsgData_t *msg, uint32_t txtime)
{
	uint8_t bcast_addr[2];
	bcast_addr[0] = 0xFF;
	bcast_addr[1] = 0xFF;
	dw_send_ieee_data_delayed(bcast_addr, msg, txtime);
}


static void dw_send_ieee_data_delayed(uint8_t* destAddr, DWR_MsgData_t *msg, uint32_t txtime)
{
		// ======== 802.15.4 Data Frame: ========

	// [15-14] Source Address mode 			1,0 (Short)
	// [13-12] Frame Version                0,0 (15.4-2003)
	// [11-10] Dest. Address mode           1,0 (Short)
	// [9-8]   Reserved                     0,0

	// [7]     Reserved                     0
	// [6]     PAN ID Compress              1 (Yes)
	// [5]     ACK Request                  0 (No)
	// [4]     Frame Pending                0 (No)
	// [3]     Security Enabled             0 (No)
	// [2-0]   Frame Type                   0,0,1 (Data)

	//	( [1] = 0x88, [0] = 0x41 )

	// data frame header (dest=short, source=short [DsSs])
	frameToSend.seqNum += 1;
	frameToSend.destAddr[0] = destAddr[0];
	frameToSend.destAddr[1] = destAddr[1];

	// data custom payload
	memcpy(frameToSend.msgData, (uint8_t*)msg, PAYLOAD_DATA_LEN);

	// write transmit binary data
	dwt_writetxdata(STANDARD_FRAME_SIZE, (uint8_t*)(&frameToSend), 0);
	// frame length w/ 2-byte CRC, offset
	dwt_writetxfctrl(STANDARD_FRAME_SIZE, 0) ;
	// set future tx time (accepts high 32 bits of 40 bit timer)
	dwt_setdelayedtrxtime(txtime);
	// schedule delayed transmission
	dwt_starttx(DWT_START_TX_DELAYED);
}

static void dw_getRxQuality(float *fppwr, float *cirp, float *fploss)
{
	uint8_t reg[2];
	float fp1, fp2, fp3, N, C;

	// --- Formula in 4.7.1 of DW1000 Manual v2.05 ---

	// get FP amplitude #1
	dwt_readfromdevice(RX_TIME_ID, 7, 2, reg);
	fp1 = reg[0] + (reg[1] << 8);

	// get FP amplitude #2 & #3
	dwt_readfromdevice(RX_FQUAL_ID, 2, 2, reg);
	fp2 = reg[0] + (reg[1] << 8);
	dwt_readfromdevice(RX_FQUAL_ID, 4, 2, reg);
	fp3 = reg[0] + (reg[1] << 8);

	// get accumulator count value
	N = (dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXPACC_MASK) >> RX_FINFO_RXPACC_SHIFT ;

	// calculate primary path power
	//*fppwr = 10*log10( (fp1*fp1 + fp2*fp2 + fp3*fp3)/(N*N)) - DWT_CONST_A_PRF64;

	// read CIR rx power
	dwt_readfromdevice(RX_FQUAL_ID, 6, 2, reg);
	C = reg[0] + (reg[1] << 8);
	*cirp = (float)C;

	// estimate path loss
	//*fploss = 10*log10( (fp1*fp1 + fp2*fp2 + fp3*fp3)/( C*POW_2_17 ) );
}


static void dw_debug(uint8_t eventType)
{
	user_config.cbDebug(eventType);
}

void decaranging_gpiotoggle_green()
{
	dwt_ntbled_toggle_green();
}

void decaranging_gpiotoggle_red()
{
	dwt_ntbled_toggle_red();
}

// project an input (raw) time into local frame, accounting for timer_overflows
static uint64_t dw_project_time(uint64_t rawtime)
{
	return timer_overflows*DWR_TIMER_MAX_40_BITS + rawtime;
}

// check a rawtime for overflows
static void dw_process_time(uint64_t rawtime)
{
	if( rawtime < time_last )
		timer_overflows++;
	time_last = rawtime;
}

// check for timer overflows
void decaranging_checkovrf()
{
	uint32_t tnow = dwt_readsystimestamphi32();
	dw_process_time( ((uint64_t)tnow)<<8 );
}

// get how many overflows there have been
uint32_t decaranging_getovrf()
{
	return timer_overflows;
}

// reset the transceiver
void decaranging_resettxr()
{
	// ensure trx is in idle state
	dwt_forcetrxoff();
	dwt_rxenable(0);
}
