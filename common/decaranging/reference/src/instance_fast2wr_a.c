/*! ----------------------------------------------------------------------------
 *  @file    instance_fast2wr_a.c
 *  @brief   DecaWave application level Fast 2W ranging demo anchor instance functions
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
//
// the main instance state machine (for Anchor instance mode only!)
//
// -------------------------------------------------------------------------------------------------------------------
//
int testapprun_af(instance_data_t *inst, int message)
{

    switch (inst->testAppState)
    {
        case TA_INIT :
            // printf("TA_INIT") ;
            switch (inst->mode)
            {
                case ANCHOR:
                {
                	uint16 shorta = (inst->eui64[0] + ((uint16)inst->eui64[1] << 8)); //assign short address
                    dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering
                    inst->frameFilteringEnabled = 0 ;
                    dwt_seteui(inst->eui64);
                    dwt_setpanid(inst->panid);
                	inst->rnmsg.panID[0] = (inst->panid) & 0xff;
                	inst->rnmsg.panID[1] = inst->panid >> 8;
                	inst->msg_f.panID[0] = (inst->panid) & 0xff;
                	inst->msg_f.panID[1] = inst->panid >> 8;

					dwt_setaddress16(shorta);

                    //set source address into the message structure
                    memcpy(&(inst->rnmsg.sourceAddr[0]), &(inst->eui64[0]), ADDR_BYTE_SIZE_S);
                    memcpy(&(inst->msg_f.sourceAddr[0]), &(inst->eui64[0]), ADDR_BYTE_SIZE_S);


                    //can use RX auto re-enable when not logging/plotting errored frames
                    inst->rxautoreenable = 1;

                    dwt_setautorxreenable(inst->rxautoreenable);

                    dwt_setdblrxbuffmode(0); //disable double RX buffer

                    dwt_setrxtimeout(0);

                    inst->tagToRangeWith = 0; //range with the first tag it sees

                    inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                    //turn RX on
                    dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1

                    inst->canprintinfo = 1;

                }
                break;
                default:
                break;
            }
            break; // end case TA_INIT


        case TA_TXE_WAIT : //either go to sleep or proceed to TX a message
            // printf("TA_TXE_WAIT") ;
            {

				inst->testAppState = inst->nextState;
               	inst->nextState = 0; //clear
            }
            break ; // end case TA_TXE_WAIT

        case TA_TXRANGINGINIT_WAIT_SEND :
                {
                //tell Tag what it's address will be for the ranging exchange
                inst->rnmsg.messageData[FCODE] = RTLS_DEMO_MSG_RNG_INIT;
                inst->rnmsg.messageData[RES_R1] = inst->tagShortAdd & 0xFF;
                inst->rnmsg.messageData[RES_R2] = (inst->tagShortAdd >> 8) & 0xFF; //
                inst->rnmsg.messageData[RES_T1] = 0 ; //((int) (inst->fixedReplyDelay_ms)) & 0xFF;
                inst->rnmsg.messageData[RES_T2] = 0 ; //(((int) (inst->fixedReplyDelay_ms)) >> 8) & 0xFF; //

                //set frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
                inst->rnmsg.frameCtrl[0] = 0x41; //PID compression, no ACK request
                //source/dest addressing modes and frame version
                inst->rnmsg.frameCtrl[1] = 0xC /*dest extended address (64bits)*/ | 0x80 /*src short address (16bits)*/;
                //set sequence number
                inst->rnmsg.seqNum = inst->frame_sn++;

                inst->wait4ack = DWT_RESPONSE_EXPECTED; //Poll is coming soon after...
                // - 6 as using short source address
                dwt_writetxfctrl((RANGINGINIT_MSG_LEN + FRAME_CRTL_AND_ADDRESS_LS + FRAME_CRC), 0);

                dwt_setdelayedtrxtime(inst->delayedReplyTime32) ;

                if(dwt_starttx(DWT_START_TX_DELAYED | inst->wait4ack))  // delayed start was too late
                {
                    //error - TX FAILED
                    inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                    //turn RX on
                    dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1
					inst->wait4ack = 0; //clear the flag as the TX has
					inst->lateTX++;
                }
                else
                {
                	inst->sentSN = inst->rnmsg.seqNum;
                	dwt_writetxdata((RANGINGINIT_MSG_LEN + FRAME_CRTL_AND_ADDRESS_LS + FRAME_CRC), (uint8 *)  &inst->rnmsg, 0) ;   // write the frame data

					inst->testAppState = TA_TX_WAIT_CONF ;                                               // wait confirmation
                    inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;  //no timeout
                }


                //while the ranging init message is going out, we can construct the response message and write it to the tx buffer

                inst->msg_f.destAddr[0] = inst->tagShortAdd & 0xff;
                inst->msg_f.destAddr[1] = (inst->tagShortAdd >> 8) & 0xff;

                //write the response function code
                inst->msg_f.messageData[FCODE] = RTLS_DEMO_MSG_ANCH_RESPF;
                //change the FC bytes for the response message
                inst->msg_f.frameCtrl[0] = 0x41;
                inst->msg_f.frameCtrl[1] = 0x88; //use short addresses

                //increment the sequence number for the response message
                inst->msg_f.seqNum = inst->frame_sn++;
                // write the final frame control and address tx data (add CRC as the function will write length - 2)
                dwt_writetxdata((FRAME_CRTL_AND_ADDRESS_S+ANCH_RESPONSE_F_MSG_LEN+FRAME_CRC), (uint8 *)  &inst->msg_f, RESPONSE_MSG_OFFSET) ;   // write the final frame data

                inst->previousState = TA_TXRANGINGINIT_WAIT_SEND ;
                inst->ackexpected = !ACK_REQUESTED ;
            }
            break;


        case TA_TXREPORT_WAIT_SEND :
            {
                if(inst->newReportSent == 0) //keep the same message if re-sending the same report
                {
                    // Write calculated TOF into report message
                	inst->msg_f.messageData[FCODE] = RTLS_DEMO_MSG_ANCH_TOFRF;
                    inst->msg_f.messageData[TOFR] = inst->tof32 & 0xff;
                    inst->msg_f.messageData[TOFR+1] = (inst->tof32 >> 8) & 0xff;
                    inst->msg_f.messageData[TOFR+2] = (inst->tof32 >> 16) & 0xff;
                    inst->msg_f.messageData[TOFR+3] = (inst->tof32 >> 24) & 0xff;

                }
                else
                {
                    //re-send the old report
                    inst->msg_f.seqNum--;
                }

                inst->psduLength = TOF_REPORT_F_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;

                //NOTE - Auto ACK only works if frame filtering is enabled!
                inst->msg_f.panID[0] = (inst->panid) & 0xff;
                inst->msg_f.panID[1] = inst->panid >> 8;

                //set frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
                inst->msg_f.frameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;
                inst->msg_f.frameCtrl[0] |= (ACK_REQUESTED ? 0x20 : 0x00);

                //short address
                inst->msg_f.frameCtrl[1] = 0x8 /*dest short address (16bits)*/ | 0x80 /*src short address (16bits)*/;

                inst->msg_f.seqNum = inst->frame_sn++;
                inst->wait4ack = DWT_RESPONSE_EXPECTED;

                inst->ackexpected = ACK_REQUESTED ; //used to ignore unexpected ACK frames

                dwt_writetxfctrl(inst->psduLength, 0);
                //dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg_f, 0) ;   // write the frame data

                dwt_starttx(DWT_START_TX_IMMEDIATE | inst->wait4ack);  //

                	inst->sentSN = inst->msg_f.seqNum;
                	dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg_f, 0) ;   // write the frame data
                    inst->testAppState = TA_TX_WAIT_CONF ;                                               // wait confirmation
                    inst->previousState = TA_TXREPORT_WAIT_SEND ;

                    inst->newReportSent++;
                    inst->delayedReplyTime32 = 0;
                    //NOTE - Auto ACK only works if frame filtering is enabled!
                    inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;

            }
            break;


        case TA_TX_WAIT_CONF :
           //printf("TA_TX_WAIT_CONF") ;
            {
            	//uint8 temp[5];
            	event_data_t *dw_event = instance_getevent(0); //get and clear this event

                //NOTE: Can get the ACK before the TX confirm event for the frame requesting the ACK
                //this happens because if polling the ISR the RX event will be processed 1st and then the TX event
                //thus the reception of the ACK will be processed before the TX confirmation of the frame that requested it.
                if(dw_event->type != DWT_SIG_TX_DONE) //wait for TX done confirmation
                {
                    if(dw_event->type == DWT_SIG_RX_TIMEOUT) //got RX timeout - i.e. did not get the response (e.g. ACK)
                    {
                    	//we need to wait for SIG_TX_DONE and then process the timeout and re-send the frame if needed
                    	inst->gotTO = 1;
                    }

                    if(dw_event->type == SIG_RX_ACK)
                    {
                        inst->wait4ack = 0 ; //clear the flag as the ACK has been received
                        inst_processackmsg(inst, dw_event->msgu.rxackmsg.seqNum);
                        //printf("RX ACK in TA_TX_WAIT_CONF... wait for TX confirm before changing state\n");
                    }

                    inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                    break;

                }

                inst->done = INST_NOT_DONE_YET;

                if(inst->previousState == TA_TXFINAL_WAIT_SEND) //tag will do immediate receive when waiting for report (as anchor sends it without delay)
                //anchor is not sending the report to tag
                {

                    inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO; //kick off the TagTimeoutTimer (instance timer) to initiate wakeup
                    inst->nextState = TA_TXPOLL_WAIT_SEND;
                    inst->testAppState = TA_TXE_WAIT; //we are going manually to sleep - change to TA_TXE_WAIT state

                    break;
                }
                else if (inst->gotTO) //timeout
                {
                	inst_processrxtimeout(inst);
                	inst->gotTO = 0;
                }
                else
                {

                    if(inst->previousState == TA_TXRANGINGINIT_WAIT_SEND) //set frame control for the response message
					{
						dwt_writetxfctrl((ANCH_RESPONSE_F_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC + EXTRA_LENGTH), RESPONSE_MSG_OFFSET);
					}

                    inst->anchorRespTxTime32l = dw_event->timeStamp32l;

#if (TWSYMRANGE == 0)
                    if(inst->previousState == TA_TXRESPONSE_WAIT_SEND)
                    {
                    	inst->delayedReplyTime32 = ((uint32)dw_event->timeStamp32h + (uint32)inst->fixedFastReplyDelay32h) ;
                    	//need to write the delayed time before starting transmission
                    	dwt_setdelayedtrxtime(inst->delayedReplyTime32) ;
                    	dwt_writetxdata((1 + FRAME_CRC), (uint8 *)  &inst->msg_f.seqNum, RESPONSE_MSG_OFFSET + 2) ;   // write the frame data
						if(dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED))
						{
							inst->lateTX++;
						}
						inst->previousState = TA_TX_WAIT_CONF;
						break;
                    }
                    else
#endif
                    {
                    inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                    }
                    //turn RX on
                    if(inst->wait4ack == 0)
                    	dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1

                    inst->wait4ack = 0 ;
                    //dwt_readfromdevice(0x19, 0, 5, temp);
                    //sprintf((char*)&usbdata[20], "T2R%d %02x%02x%02x%02x%02x ", count, temp[4], temp[3], temp[2], temp[1], temp[0]);
                    //send_usbmessage(&usbdata[20], 16);

                    //count=0;
                }

            }

            break ; // end case TA_TX_WAIT_CONF

        case TA_RXE_WAIT :
			//printf("TA_RXE_WAIT") ;
			{
				// - with "fast" ranging - we only get here after frame timeout...
				//turn RX on
				instancerxon(inst, 0, 0) ;   // turn RX on, with/without delay

				inst->testAppState = TA_RX_WAIT_DATA;   // let this state handle it

				// end case TA_RXE_WAIT, don't break, but fall through into the TA_RX_WAIT_DATA state to process it immediately.
				if(message == 0) break;
			}

        case TA_RX_WAIT_DATA :                                                                     // Wait RX data
           //printf("TA_RX_WAIT_DATA") ;

            switch (message)
            {

                case SIG_RX_BLINK :
                {
                	event_data_t* dw_event = instance_getevent(1); //get and clear this event

					//add this Tag to the list of Tags we know about
					instaddtagtolist(inst, &(dw_event->msgu.rxblinkmsg.tagID[0]));

					//initiate ranging message
					if(inst->tagToRangeWith < TAG_LIST_SIZE)
					{
						//initiate ranging message this is a Blink from the Tag we would like to range to
						if(memcmp(&inst->tagList[inst->tagToRangeWith][0],  &(dw_event->msgu.rxblinkmsg.tagID[0]), BLINK_FRAME_SOURCE_ADDRESS) == 0)
						{
							inst->tagShortAdd = (dwt_getpartid() & 0xFF);
							inst->tagShortAdd =  ((uint16)inst->tagShortAdd << 8) + (uint16)dw_event->msgu.rxblinkmsg.tagID[0] ;

							inst->delayedReplyTime32 = (uint32)dw_event->timeStamp32h + (uint32)inst->fixedRngInitReplyDelay32h ;  // time we should send the blink response

							memcpy(&(inst->rnmsg.destAddr[0]), &(dw_event->msgu.rxblinkmsg.tagID[0]), BLINK_FRAME_SOURCE_ADDRESS); //remember who to send the reply to;

							inst->testAppState = TA_TXRANGINGINIT_WAIT_SEND ;

							break;
						}

						//else stay in RX
					}

                    //else //not initiating ranging - continue to receive
                    {
                        inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                        //turn RX on
                        dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1
                        inst->done = INST_NOT_DONE_YET;
                    }

                }
                break;

                case SIG_RX_ACK :
                {
					event_data_t* dw_event = instance_getevent(2); //get and clear this event
					inst_processackmsg(inst, dw_event->msgu.rxackmsg.seqNum);
                    //else we did not expect this ACK turn the RX on again
                    //only enable receiver when not using double buffering
                    inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                    //turn RX on
                    dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1
                    inst->done = INST_NOT_DONE_YET;
                }
                break;


				case DWT_SIG_RX_OKAY :
				{
					event_data_t* dw_event = instance_getevent(3); //get and clear this event
					uint8  srcAddr[8] = {0,0,0,0,0,0,0,0};
					int fcode = 0;
					int fn_code = 0;
					int srclen = 0;
					int fctrladdr_len;
					uint8 *messageData;

					inst->stoptimer = 0; //clear the flag, as we have received a message

					// 16 or 64 bit addresses
					switch(dw_event->msgu.frame[1])
					{
						case 0xCC: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ll.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event->msgu.rxmsg_ll.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ll.messageData[0];
							srclen = ADDR_BYTE_SIZE_L;
							fctrladdr_len = FRAME_CRTL_AND_ADDRESS_L;
							break;
						case 0xC8: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_sl.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event->msgu.rxmsg_sl.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_sl.messageData[0];
							srclen = ADDR_BYTE_SIZE_L;
							fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x8C: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ls.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event->msgu.rxmsg_ls.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ls.messageData[0];
							srclen = ADDR_BYTE_SIZE_S;
							fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x88: //
							memcpy(&srcAddr[0], &(dw_event->msgu.rxmsg_ss.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event->msgu.rxmsg_ss.messageData[FCODE];
							messageData = &dw_event->msgu.rxmsg_ss.messageData[0];
							srclen = ADDR_BYTE_SIZE_S;
							fctrladdr_len = FRAME_CRTL_AND_ADDRESS_S;
							break;
					}

					if((inst->ackexpected) && (inst->ackTO)) //ACK frame was expected but we got a good frame - treat as ACK timeout
					{
						//printf("got good frame instead of ACK in DWT_SIG_RX_OKAY - pretend TO\n");
						inst_processrxtimeout(inst);
						message = 0; //clear the message as we have processed the event
					}
                    else
                    {

                    	inst->ackexpected = 0; //clear this as we got good frame (but as not using ACK TO) we prob missed the ACK - check if it has been addressed to us

                    	//if using 16-bit addresses the ranging messages from tag are using the short address tag was given in the ranging init message
                    	if(inst->tagShortAdd == (uint16)(srcAddr[0] + ((uint16)srcAddr[1] << 8)))
						//only process messages from the associated tag
						{
							fcode = fn_code;
						}

                        switch(fcode)
                        {

                            case RTLS_DEMO_MSG_TAG_POLLF:
                            {
                            	//uint8 temp[5];
                            	uint8 late = 0;
                            	inst->delayedReplyTime32 = ((uint32)dw_event->timeStamp32h + (uint32)inst->fixedFastReplyDelay32h) ;
								//need to write the delayed time before starting transmission
								dwt_setdelayedtrxtime(inst->delayedReplyTime32) ;

								//copy the source address
								inst->msg_f.destAddr[0] = srcAddr[0];
								inst->msg_f.destAddr[1] = srcAddr[1];
#if (TWSYMRANGE == 1)
								dwt_writetxdata((1 + FRAME_CRC), (uint8 *)  &inst->msg_f.seqNum, RESPONSE_MSG_OFFSET + 2) ;   // write the frame data

								if(dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED)) //
#else
								//send 9 LSBs from the TX time
								//write SN, PANID, DEST, SRC, + 3 bytes, CRC
								inst->msg_f.messageData[1] = (uint8) dw_event->timeStamp32l & 0xff;
								inst->msg_f.messageData[2] = (uint8) dw_event->timeStamp32h & 0x01;
								dwt_writetxdata((10 + FRAME_CRC), (uint8 *)  &inst->msg_f.seqNum, RESPONSE_MSG_OFFSET + 2) ;   // write the frame data

								if(dwt_starttx(DWT_START_TX_DELAYED)) //
#endif
								{
									//uint8 temp[5];
									//error - TX FAILED
									inst->testAppState = TA_RX_WAIT_DATA ;  // wait to receive a new poll
		                            //turn RX on
									dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1
									inst->wait4ack = 0; //clear the flag as the TX has
									inst->lateTX++;
	                                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;

	                                late = 1;
	                                //dwt_readfromdevice(0x19, 0, 5, temp);
	                                //sprintf((char*)&usbdata[0], " %02x%02x%02x%02x%02x", temp[4], temp[3], temp[2], temp[1], temp[0]);
	                                //send_usbmessage(&usbdata[0], 11);
								}
								else
								{
									inst->sentSN = inst->msg_f.seqNum;
#if (TWSYMRANGE == 1)
									inst->wait4ack = DWT_RESPONSE_EXPECTED; //Final is coming after 500 us...
#else
									inst->wait4ack = 0 ;
#endif
									//use delayed rx on (wait4resp timer) - this value is applied when the TX frame is done/sent, so this value can be written after TX is started
									dwt_setrxaftertxdelay(inst->fixedReplyDelay_sy);  //units are ~us - wait for wait4respTIM before RX on (delay RX)
									//if the response is expected there is a 1ms timeout to stop RX if no response (ACK or other frame) coming
									//dwt_setrxtimeout(0);  //units are us - wait for 2ms after RX on

									inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;  //no timeout
									inst->testAppState = TA_TX_WAIT_CONF;        // wait confirmation - so we can read the tx timestamp

									inst->msg_f.seqNum = inst->frame_sn++; //increment as response is sent
									inst->tagPollRxTime32l = dw_event->timeStamp32l; //Save Poll's Rx time for ToF calculation
								}
                                //dwt_readfromdevice(0x19, 0, 5, temp);
                                //sprintf((char*)&usbdata[0], "%d %02x%02x%02x%02x%02x", late, temp[4], temp[3], temp[2], temp[1], temp[0]);
                                //sprintf((char*)&usbdata[12], " %llx", inst->rxTimeStamp);
                                //sprintf((char*)&usbdata[10], "%15d %04d %04d", portGetTickCount(), (dwt_read16bitoffsetreg(0x15, 5) >> 6), (dwt_read16bitoffsetreg(0x10, 2) >> 4));
                                //sprintf((char*)&usbdata[35], " %04d %04d", dwt_read16bitoffsetreg(0x2e, 0x100c), dwt_read16bitoffsetreg(0x2e, 0x0));
                                //send_usbmessage(&usbdata[0], 45);

								inst->ackexpected = !ACK_REQUESTED ; //used to ignore unexpected ACK frames
								inst->previousState = TA_TXRESPONSE_WAIT_SEND ;
								if (!inst->frameFilteringEnabled) //check if Frame Filtering is enabled (we want it ON when ranging)
								{
									// if we missed the ACK to the ranging init message we may not have turned frame filtering on
									dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //we are starting ranging - enable the filter....
									inst->frameFilteringEnabled = 1 ;
								}
                            }
                            break; //RTLS_DEMO_MSG_TAG_POLLF

                            case RTLS_DEMO_MSG_ANCH_TOFRF:
                            {
                                    inst->tof32 = (uint32)messageData[TOFR];
                                    inst->tof32 += (uint32)messageData[TOFR+1] << 8;
                                    inst->tof32 += (uint32)messageData[TOFR+2] << 16;
                                    inst->tof32 += (uint32)messageData[TOFR+3] << 24;

                                    if(dw_event->msgu.rxmsg_ss.seqNum != inst->lastReportSN)
                                    {
                                        reportTOF(inst);
                                        inst->newrange = 1;
                                        inst->lastReportSN = dw_event->msgu.rxmsg_ss.seqNum;
                                    }

                                    inst->testAppState = TA_TXE_WAIT;
                                    inst->nextState = TA_TXPOLL_WAIT_SEND ; // send next poll

                            }
                            break; //RTLS_DEMO_MSG_ANCH_TOFRF

                            case RTLS_DEMO_MSG_TAG_FINALF:
                            {
                                uint32 tRxT, aTxT ;
                                rtd_t rtd;

                                uint32 pollRespRTD  = 0;
                                uint32 respFinalRTD  = 0;

                                // time of arrival of Final message
                                inst->delayedReplyTime32 = 0 ; //no delay if sending the report

                                memcpy((uint8*)&rtd, &(messageData[1]), 8);

                                // poll response round trip delay time is calculated as
                                // (anchorRespRxTime - tagPollTxTime) - (anchorRespTxTime - tagPollRxTime)
                                aTxT = ((uint32)inst->anchorRespTxTime32l - (uint32)inst->tagPollRxTime32l);
                                pollRespRTD = ((uint32)rtd.diffRmP - (uint32)aTxT);


                                // response final round trip delay time is calculated as
                                // (tagFinalRxTime - anchorRespTxTime) - (tagFinalTxTime - anchorRespRxTime)
                                tRxT = ((uint32)dw_event->timeStamp32l - (uint32)inst->anchorRespTxTime32l);
                                respFinalRTD = ((uint32)tRxT - (uint32)rtd.diffFmR);

                                // add both round trip delay times
                                inst->tof32 = ((uint32)pollRespRTD + (uint32)respFinalRTD);

                                reportTOF_f(inst);
                                inst->newrange = 1;

                                //only enable receiver when not using double buffering
                                inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                                //turn RX on
                                dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1

                            }
                            break; //RTLS_DEMO_MSG_TAG_FINALF


                            default:
                            {
                                //only enable receiver when not using double buffering
                                inst->testAppState = TA_RX_WAIT_DATA ;              // wait for next frame
                                //turn RX on
                                dwt_rxenable(0) ;               // turn receiver on, immediate = 0/delayed = 1

                            }
                            break;
                        } //end switch (rxmsg->functionCode)

    					if(dw_event->msgu.frame[0] & 0x20)
    					{
    						//as we only pass the received frame with the ACK request bit set after the ACK has been sent
    						instance_getevent(4); //get and clear the ACK sent event
    					}

                    }
                }
                break ;

                case DWT_SIG_RX_TIMEOUT :
                    //printf("PD_DATA_TIMEOUT") ;
                	instance_getevent(27); //get and clear this event
                	inst_processrxtimeout(inst);
                    message = 0; //clear the message as we have processed the event
                break ;

                case DWT_SIG_TX_AA_DONE: //ignore this event - just process the rx frame that was received before the ACK response
                case 0: //no event - wait in receive...
                {
                    //stay in Rx (fall-through from above state)
                    //if(DWT_SIG_TX_AA_DONE == message) printf("Got SIG_TX_AA_DONE in RX wait - ignore\n");
                    if(inst->done == INST_NOT_DONE_YET) inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
                }
                break;

                default :
                {
                    //printf("\nERROR - Unexpected message %d ??\n", message) ;
                    //assert(0) ;                                             // Unexpected Primitive what is going on ?
                }
                break ;

            }
            break ; // end case TA_RX_WAIT_DATA

            default:
                //printf("\nERROR - invalid state %d - what is going on??\n", inst->testAppState) ;
            break;
    } // end switch on testAppState

    return inst->done;
} // end testapprun()

// -------------------------------------------------------------------------------------------------------------------
#if NUM_INST != 1
#error These functions assume one instance only
#endif


/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
