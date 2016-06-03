/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   main loop for the DecaRanging application
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
/* Includes */
#include "compiler.h"
#include "port.h"

#include "instance.h"

#include "deca_types.h"

#include "deca_spi.h"

extern void usb_run(void);
extern int usb_init(void);
extern void usb_printconfig(void);
extern void send_usbmessage(uint8*, int);

#define SOFTWARE_VER_STRING    "Version 2.35    " //


int instance_anchaddr = 0; //0 = 0xDECA020000000001; 1 = 0xDECA020000000002; 2 = 0xDECA020000000003
int dr_mode = 0;
//if instance_mode = TAG_TDOA then the device cannot be selected as anchor
int instance_mode = ANCHOR;
//int instance_mode = TAG;
//int instance_mode = TAG_TDOA;
//int instance_mode = LISTENER;
int paused = 0;

double antennaDelay  ;                          // This is system effect on RTD subtracted from local calculation.


char reset_request;

typedef struct
{
    uint8 channel ;
    uint8 prf ;
    uint8 datarate ;
    uint8 preambleCode ;
    uint8 preambleLength ;
    uint8 pacSize ;
    uint8 nsSFD ;
    uint16 sfdTO ;
} chConfig_t ;


//Configuration for DecaRanging Modes (8 default use cases selectable by the switch S1 on EVK)
chConfig_t chConfig[8] ={
                    //mode 1 - S1: 7 off, 6 off, 5 off
                    {
                        2,              // channel
                        DWT_PRF_16M,    // prf
                        DWT_BR_110K,    // datarate
                        3,             // preambleCode
                        DWT_PLEN_1024,  // preambleLength
                        DWT_PAC32,      // pacSize
                        1,       // non-standard SFD
                        (1025 + 64 - 32) //SFD timeout
                    },
                    //mode 2
                    {
                        2,              // channel
                        DWT_PRF_16M,    // prf
                        DWT_BR_6M8,    // datarate
                        3,             // preambleCode
                        DWT_PLEN_128,   // preambleLength
                        DWT_PAC8,       // pacSize
                        0,       // non-standard SFD
                        (129 + 8 - 8) //SFD timeout
                    },
                    //mode 3
                    {
                        2,              // channel
                        DWT_PRF_64M,    // prf
                        DWT_BR_110K,    // datarate
                        9,             // preambleCode
                        DWT_PLEN_1024,  // preambleLength
                        DWT_PAC32,      // pacSize
                        1,       // non-standard SFD
                        (1025 + 64 - 32) //SFD timeout
                    },
                    //mode 4
                    {
                        2,              // channel
                        DWT_PRF_64M,    // prf
                        DWT_BR_6M8,    // datarate
                        9,             // preambleCode
                        DWT_PLEN_128,   // preambleLength
                        DWT_PAC8,       // pacSize
                        0,       // non-standard SFD
                        (129 + 8 - 8) //SFD timeout
                    },
                    //mode 5
                    {
                        5,              // channel
                        DWT_PRF_16M,    // prf
                        DWT_BR_110K,    // datarate
                        3,             // preambleCode
                        DWT_PLEN_1024,  // preambleLength
                        DWT_PAC32,      // pacSize
                        1,       // non-standard SFD
                        (1025 + 64 - 32) //SFD timeout
                    },
                    //mode 6
                    {
                        5,              // channel
                        DWT_PRF_16M,    // prf
                        DWT_BR_6M8,    // datarate
                        3,             // preambleCode
                        DWT_PLEN_128,   // preambleLength
                        DWT_PAC8,       // pacSize
                        0,       // non-standard SFD
                        (129 + 8 - 8) //SFD timeout
                    },
                    //mode 7
                    {
                        5,              // channel
                        DWT_PRF_64M,    // prf
                        DWT_BR_110K,    // datarate
                        9,             // preambleCode
                        DWT_PLEN_1024,  // preambleLength
                        DWT_PAC32,      // pacSize
                        1,       // non-standard SFD
                        (1025 + 64 - 32) //SFD timeout
                    },
                    //mode 8
                    {
                        5,              // channel
                        DWT_PRF_64M,    // prf
                        DWT_BR_6M8,    // datarate
                        9,             // preambleCode
                        DWT_PLEN_128,   // preambleLength
                        DWT_PAC8,       // pacSize
                        0,       // non-standard SFD
                        (129 + 8 - 8) //SFD timeout
                    }
};


#if (DR_DISCOVERY == 0)
//Tag address list
uint64 tagAddressList[3] =
{
     0xDECA010000001001,         // First tag
     0xDECA010000000002,         // Second tag
     0xDECA010000000003          // Third tag
} ;

//Anchor address list
uint64 anchorAddressList[ANCHOR_LIST_SIZE] =
{
     0xDECA020000000001 ,       // First anchor
     0xDECA020000000002 ,       // Second anchor
     0xDECA020000000003 ,       // Third anchor
     0xDECA020000000004         // Fourth anchor
} ;

//ToF Report Forwarding Address
uint64 forwardingAddress[1] =
{
     0xDECA030000000001
} ;


// ======================================================
//
//  Configure instance tag/anchor/etc... addresses
//
void addressconfigure(void)
{
    instanceAddressConfig_t ipc ;

    ipc.forwardToFRAddress = forwardingAddress[0];
    ipc.anchorAddress = anchorAddressList[instance_anchaddr];
    ipc.anchorAddressList = anchorAddressList;
    ipc.anchorListSize = ANCHOR_LIST_SIZE ;
    ipc.anchorPollMask = 0x1; //0x7;              // anchor poll mask

    ipc.sendReport = 1 ;  //1 => anchor sends TOF report to tag
    //ipc.sendReport = 2 ;  //2 => anchor sends TOF report to listener

    instancesetaddresses(&ipc);
}
#endif

uint32 inittestapplication(void);

// Restart and re-configure
void restartinstance(void)
{
    instance_close() ;                          //shut down instance, PHY, SPI close, etc.

    spi_peripheral_init();                      //re initialise SPI...

    inittestapplication() ;                     //re-initialise instance/device
} // end restartinstance()



int decarangingmode(void)
{
    int mode = 0;

    if(is_switch_on(TA_SW1_5))
    {
        mode = 1;
    }

    if(is_switch_on(TA_SW1_6))
    {
        mode = mode + 2;
    }

    if(is_switch_on(TA_SW1_7))
    {
        mode = mode + 4;
    }

    return mode;
}

uint32 inittestapplication(void)
{
    uint32 devID ;
    instanceConfig_t instConfig;
    int i , result;

    SPI_ConfigFastRate(SPI_BaudRatePrescaler_16);  //max SPI before PLLs configured is ~4M

    i = 10;

    //this is called here to wake up the device (i.e. if it was in sleep mode before the restart)
    devID = instancereaddeviceid() ;
    if(DWT_DEVICE_ID != devID) //if the read of devide ID fails, the DW1000 could be asleep
    {
        port_SPIx_clear_chip_select();  //CS low
        Sleep(1);   //200 us to wake up then waits 5ms for DW1000 XTAL to stabilise
        port_SPIx_set_chip_select();  //CS high
        Sleep(7);
        devID = instancereaddeviceid() ;
        // SPI not working or Unsupported Device ID
        if(DWT_DEVICE_ID != devID)
            return(-1) ;
        //clear the sleep bit - so that after the hard reset below the DW does not go into sleep
        dwt_softreset();
    }

    //reset the DW1000 by driving the RSTn line low
    reset_DW1000();

    result = instance_init() ;
    if (0 > result) return(-1) ; // Some failure has occurred

    SPI_ConfigFastRate(SPI_BaudRatePrescaler_4); //increase SPI to max
    devID = instancereaddeviceid() ;

    if (DWT_DEVICE_ID != devID)   // Means it is NOT MP device
    {
        // SPI not working or Unsupported Device ID
        return(-1) ;
    }

    if(port_IS_TAG_pressed() == 0)
    {
        instance_mode = TAG;
        led_on(LED_PC7);
    }
    else
    {
        instance_mode = ANCHOR;
#if (DR_DISCOVERY == 1)
        led_on(LED_PC6);
#else
        if(instance_anchaddr & 0x1)
            led_on(LED_PC6);

        if(instance_anchaddr & 0x2)
            led_on(LED_PC8);
#endif
    }

    instancesetrole(instance_mode) ;     // Set this instance role

    if(is_fastrng_on(0) == S1_SWITCH_ON) //if fast ranging then initialise instance for fast ranging application
    {
        instance_init_f(instance_mode); //initialise Fast 2WR specific data
        //when using fast ranging the channel config is either mode 2 or mode 6
        //default is mode 2
        dr_mode = decarangingmode();

        if((dr_mode & 0x1) == 0)
            dr_mode = 1;
    }
    else
    {
        instance_init_s(instance_mode);
    dr_mode = decarangingmode();
    }

    instConfig.channelNumber = chConfig[dr_mode].channel ;
    instConfig.preambleCode = chConfig[dr_mode].preambleCode ;
    instConfig.pulseRepFreq = chConfig[dr_mode].prf ;
    instConfig.pacSize = chConfig[dr_mode].pacSize ;
    instConfig.nsSFD = chConfig[dr_mode].nsSFD ;
    instConfig.sfdTO = chConfig[dr_mode].sfdTO ;
    instConfig.dataRate = chConfig[dr_mode].datarate ;
    instConfig.preambleLen = chConfig[dr_mode].preambleLength ;

    instance_config(&instConfig) ;                  // Set operating channel etc

#if (DR_DISCOVERY == 0)
    addressconfigure() ;                            // set up initial payload configuration
#endif
    instancesettagsleepdelay(POLL_SLEEP_DELAY, BLINK_SLEEP_DELAY); //set the Tag sleep time

    //if TA_SW1_2 is on use fast ranging (fast 2wr)
    if(is_fastrng_on(0) == S1_SWITCH_ON)
    {
        //Fast 2WR specific config
        //configure the delays/timeouts
        instance_config_f();
    }
    else //use default ranging modes
    {
        // NOTE: this is the delay between receiving the blink and sending the ranging init message
        // The anchor ranging init response delay has to match the delay the tag expects
        // the tag will then use the ranging response delay as specified in the ranging init message
        // use this to set the long blink response delay (e.g. when ranging with a PC anchor that wants to use the long response times != 150ms)
        if(is_switch_on(TA_SW1_8) == S1_SWITCH_ON)
        {
            instancesetblinkreplydelay(FIXED_LONG_BLINK_RESPONSE_DELAY);
        }
        else //this is for ARM to ARM tag/anchor (using normal response times 150ms)
        {
            instancesetblinkreplydelay(FIXED_REPLY_DELAY);
        }

        //set the default response delays
        instancesetreplydelay(FIXED_REPLY_DELAY, 0);
    }

    return devID;
}
/**
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/
void process_dwRSTn_irq(void)
{
    instance_notify_DW1000_inIDLE(1);
}

void process_deca_irq(void)
{
    do{

        instance_process_irq(0);

    }while(port_CheckEXT_IRQ() == 1); //while IRS line active (ARM can only do edge sensitive interrupts)

}

void initLCD(void)
{
    uint8 initseq[9] = { 0x39, 0x14, 0x55, 0x6D, 0x78, 0x38 /*0x3C*/, 0x0C, 0x01, 0x06 };
    uint8 command = 0x0;
    int j = 100000;

    writetoLCD( 9, 0,  initseq); //init seq
    while(j--);

    command = 0x2 ;  //return cursor home
    writetoLCD( 1, 0,  &command);
    command = 0x1 ;  //clear screen
    writetoLCD( 1, 0,  &command);
}

/*
 * @brief switch_mask  - bitmask of testing switches (currently 7 switches)
 *        switchbuff[] - switch name to test
 *        *switch_fn[]() - corresponded to switch test function
**/
#define switch_mask   (0x7F)

const uint8 switchbuf[]={0, TA_SW1_3 , TA_SW1_4 , TA_SW1_5 , TA_SW1_6 , TA_SW1_7 , TA_SW1_8 };
const int (* switch_fn[])(uint16)={ &is_button_low, \
                                &is_switch_on, &is_switch_on, &is_switch_on,\
                                &is_switch_on, &is_switch_on, &is_switch_on };

/*
 * @fn test_application_run
 * @brief   test application for production pre-test procedure
**/
void test_application_run(void)
{
    char  dataseq[2][40];
    uint8 j, switchStateOn, switchStateOff;

    switchStateOn=0;
    switchStateOff=0;

    led_on(LED_ALL);    // show all LED OK
    Sleep(1000);

    dataseq[0][0] = 0x1 ;  //clear screen
    writetoLCD( 1, 0, (const uint8 *) &dataseq);
    dataseq[0][0] = 0x2 ;  //return cursor home
    writetoLCD( 1, 0, (const uint8 *) &dataseq);

/* testing SPI to DW1000*/
    writetoLCD( 40, 1, (const uint8 *) "TESTING         ");
    writetoLCD( 40, 1, (const uint8 *) "SPI, U2, S2, S3 ");
    Sleep(1000);

    if(inittestapplication() == (uint32)-1)
    {
        writetoLCD( 40, 1, (const uint8 *) "SPI, U2, S2, S3 ");
        writetoLCD( 40, 1, (const uint8 *) "-- TEST FAILS --");
        while(1); //stop
    }

    writetoLCD( 40, 1, (const uint8 *) "SPI, U2, S2, S3 ");
    writetoLCD( 40, 1, (const uint8 *) "    TEST OK     ");
    Sleep(1000);

/* testing of switch S2 */
    dataseq[0][0] = 0x1 ;  //clear screen
    writetoLCD( 1, 0, (const uint8 *) &dataseq);

    while( (switchStateOn & switchStateOff) != switch_mask )
        {
        memset(&dataseq, ' ', sizeof(dataseq));
        strcpy(&dataseq[0][0], (const char *)"SWITCH");
        strcpy(&dataseq[1][0], (const char *)"toggle");
//switch 7-1
        for (j=0;j<sizeof(switchbuf);j++)
        {
            if( switch_fn[j](switchbuf[j]) ) //execute current switch switch_fn
            {
                dataseq[0][8+j]='O';
                switchStateOn |= 0x01<<j;
                switchStateOff &= ~(0x01<<j);//all switches finaly should be in off state
            }else{
                dataseq[1][8+j]='O';
                switchStateOff |=0x01<<j;
        }
        }

        writetoLCD(40, 1, (const uint8 *) &dataseq[0][0]);
        writetoLCD(40, 1, (const uint8 *) &dataseq[1][0]);
        Sleep(100);
        }

    led_off(LED_ALL);

    writetoLCD( 40, 1, (const uint8 *) "  Preliminary   ");
    writetoLCD( 40, 1, (const uint8 *) "   TEST OKAY    ");

    while(1);
    }


/*
 * @fn      main()
 * @brief   main entry point
**/
int main(void)
{
    int i = 0;
    int toggle = 1;
    int ranging = 0;
    uint8 dataseq[40];
    double range_result = 0;
    double avg_result = 0;
    uint8 dataseq1[40];

    led_off(LED_ALL); //turn off all the LEDs

    peripherals_init();

    spi_peripheral_init();

    Sleep(1000); //wait for LCD to power on

    initLCD();

    memset(dataseq, 40, 0);
    memcpy(dataseq, (const uint8 *) "DECAWAVE        ", 16);
    writetoLCD( 40, 1, dataseq); //send some data
    memcpy(dataseq, (const uint8 *) SOFTWARE_VER_STRING, 16); // Also set at line #26 (Should make this from single value !!!)
    writetoLCD( 16, 1, dataseq); //send some data

    Sleep(1000);
#ifdef USB_SUPPORT
    // enable the USB functionality
    usb_init();
    Sleep(1000);
#endif


    port_DisableEXT_IRQ(); //disable ScenSor IRQ until we configure the device

    //test EVB1000 - used in EVK1000 production
#if 1
    if((is_button_low(0) == S1_SWITCH_ON) && (is_switch_on(TA_SW1_8) == S1_SWITCH_ON)) //using BOOT1 switch for test
    {
        test_application_run(); //does not return....
    }
    else
#endif
    if(is_switch_on(TA_SW1_3) == S1_SWITCH_OFF)
    {
        int j = 1000000;
        uint8 command;

        memset(dataseq, 0, 40);

        while(j--);
        //command = 0x1 ;  //clear screen
        //writetoLCD( 1, 0,  &command);
        command = 0x2 ;  //return cursor home
        writetoLCD( 1, 0,  &command);

        memcpy(dataseq, (const uint8 *) "DECAWAVE   ", 12);
        writetoLCD( 40, 1, dataseq); //send some data
#ifdef USB_SUPPORT //this is set in the port.h file
        memcpy(dataseq, (const uint8 *) "USB to SPI ", 12);
#else
#endif
        writetoLCD( 16, 1, dataseq); //send some data

        j = 1000000;

        while(j--);

        command = 0x2 ;  //return cursor home
        writetoLCD( 1, 0,  &command);
#ifdef USB_SUPPORT //this is set in the port.h file
        // enable the USB functionality
        //usb_init();

        NVIC_DisableDECAIRQ();

        // Do nothing in foreground -- allow USB application to run, I guess on the basis of USB interrupts?
        while (1)       // loop forever
        {
            usb_run();
        }
#endif
        return 1;
    }
    else //run DecaRanging application
    {
        uint8 dataseq[40];
        uint8 command = 0x0;

        command = 0x2 ;  //return cursor home
        writetoLCD( 1, 0,  &command);
        memset(dataseq, ' ', 40);
        memcpy(dataseq, (const uint8 *) "DECAWAVE  RANGE", 15);
        writetoLCD( 15, 1, dataseq); //send some data

        led_off(LED_ALL);

#ifdef USB_SUPPORT //this is set in the port.h file
        usb_printconfig();
#endif

        if(inittestapplication() == (uint32)-1)
        {
            led_on(LED_ALL); //to display error....
            dataseq[0] = 0x2 ;  //return cursor home
            writetoLCD( 1, 0,  &dataseq[0]);
            memset(dataseq, ' ', 40);
            memcpy(dataseq, (const uint8 *) "ERROR   ", 12);
            writetoLCD( 40, 1, dataseq); //send some data
            memcpy(dataseq, (const uint8 *) "  INIT FAIL ", 12);
            writetoLCD( 40, 1, dataseq); //send some data
            return 0; //error
        }

        //sleep for 5 seconds displaying "Decawave"
        i=30;
        while(i--)
        {
            if (i & 1) led_off(LED_ALL);
            else    led_on(LED_ALL);

            Sleep(200);
        }
        i = 0;
        led_off(LED_ALL);
        command = 0x2 ;  //return cursor home
        writetoLCD( 1, 0,  &command);

        memset(dataseq, ' ', 40);

        if(port_IS_TAG_pressed() == 0)
        {
            instance_mode = TAG;
            led_on(LED_PC7);
        }
        else
        {
            instance_mode = ANCHOR;
#if (DR_DISCOVERY == 1)
            led_on(LED_PC6);
#else
            if(instance_anchaddr & 0x1)
                led_on(LED_PC6);

            if(instance_anchaddr & 0x2)
                led_on(LED_PC8);
#endif
        }

        if(instance_mode == TAG)
        {
            //if TA_SW1_2 is on use fast ranging (fast 2wr)
            if(is_button_low(0) == S1_SWITCH_ON)
            {
                memcpy(&dataseq[2], (const uint8 *) " Fast Tag   ", 12);
                writetoLCD( 40, 1, dataseq); //send some data
                memcpy(&dataseq[2], (const uint8 *) "   Ranging  ", 12);
                writetoLCD( 16, 1, dataseq); //send some data
            }
            else
            {
                memcpy(&dataseq[2], (const uint8 *) " TAG BLINK  ", 12);
                writetoLCD( 40, 1, dataseq); //send some data
                sprintf((char*)&dataseq[0], "%llX", instance_get_addr());
                writetoLCD( 16, 1, dataseq); //send some data
            }
        }
        else
        {
            memcpy(&dataseq[2], (const uint8 *) "  AWAITING  ", 12);
            writetoLCD( 40, 1, dataseq); //send some data
            memcpy(&dataseq[2], (const uint8 *) "    POLL    ", 12);
            writetoLCD( 16, 1, dataseq); //send some data
        }

        command = 0x2 ;  //return cursor home
        writetoLCD( 1, 0,  &command);
    }

    port_EnableEXT_IRQ(); //enable ScenSor IRQ before starting

    memset(dataseq, ' ', 40);
    memset(dataseq1, ' ', 40);
    // main loop
    while(1)
    {
        instance_run();

        if(instancenewrange())
        {
            ranging = 1;
            //send the new range information to LCD and/or USB
            range_result = instance_get_idist();
#if (DR_DISCOVERY == 0)
            if(instance_mode == ANCHOR)
#endif
                avg_result = instance_get_adist();
            //set_rangeresult(range_result);
            dataseq[0] = 0x2 ;  //return cursor home
            writetoLCD( 1, 0,  dataseq);

            memset(dataseq, ' ', 40);
            memset(dataseq1, ' ', 40);
            sprintf((char*)&dataseq[1], "LAST: %4.2f m", range_result);
            writetoLCD( 40, 1, dataseq); //send some data

#if (DR_DISCOVERY == 0)
            if(instance_mode == ANCHOR)
                sprintf((char*)&dataseq1[1], "AVG8: %4.2f m", avg_result);
            else
                sprintf((char*)&dataseq1[0], "%llx", instance_get_anchaddr());
#else
            sprintf((char*)&dataseq1[1], "AVG8: %4.2f m", avg_result);
#endif
            writetoLCD( 16, 1, dataseq1); //send some data
#ifdef USB_SUPPORT //this is set in the port.h file
            if(instance_mode == TAG)
            {
                sprintf((char*)&dataseq[0], "t %4.2f %4d %4d %04d %d %d", range_result, instance_get_lcount(), instance_get_dly(), instance_get_respPSC(), instance_get_txl(), instance_get_rxl());
                //sprintf((char*)&dataseq[20], " %08x", dwt_read32bitoffsetreg(0xa, 1));
            }
            else
            {
                sprintf((char*)&dataseq[0], "a %4.2f %4d %4d %04d %d %d", range_result, instance_get_lcount(), instance_get_dly(), (dwt_read16bitoffsetreg(0x10, 2) >> 4), instance_get_txl(), instance_get_rxl());
            }
            send_usbmessage(&dataseq[0], 35);
#endif
        }

        if(ranging == 0)
        {
            if(instance_mode != ANCHOR)
            {
                if(instancesleeping())
                {
                    dataseq[0] = 0x2 ;  //return cursor home
                    writetoLCD( 1, 0,  dataseq);
                    if(toggle)
        {
                        toggle = 0;
                        memcpy(&dataseq[0], (const uint8 *) "    AWAITING    ", 16);
                        writetoLCD( 40, 1, dataseq); //send some data
                        memcpy(&dataseq[0], (const uint8 *) "    RESPONSE    ", 16);
                        writetoLCD( 16, 1, dataseq); //send some data
                    }
                    else
                    {
                        toggle = 1;
                        memcpy(&dataseq[0], (const uint8 *) "   TAG BLINK    ", 16);
                        writetoLCD( 40, 1, dataseq); //send some data
                        sprintf((char*)&dataseq[0], "%llX", instance_get_addr());
                        writetoLCD( 16, 1, dataseq); //send some data
                    }
//#ifdef USB_SUPPORT //this is set in the port.h file
                    //send_usbmessage(&dataseq[0], 35);
//#endif
                }

                if(instanceanchorwaiting() == 2)
                {
                    ranging = 1;
                    dataseq[0] = 0x2 ;  //return cursor home
                    writetoLCD( 1, 0,  dataseq);
                    memcpy(&dataseq[0], (const uint8 *) "    RANGING WITH", 16);
                    writetoLCD( 40, 1, dataseq); //send some data
                    sprintf((char*)&dataseq[0], "%016llX", instance_get_anchaddr());
                    writetoLCD( 16, 1, dataseq); //send some data
                }
            }
            else
            {
                if(instanceanchorwaiting())
                {
                    toggle+=2;

                    if(toggle > 300000)
                    {
                        dataseq[0] = 0x2 ;  //return cursor home
                        writetoLCD( 1, 0,  dataseq);
                        if(toggle & 0x1)
                        {
                            toggle = 0;
                            memcpy(&dataseq[0], (const uint8 *) "    AWAITING    ", 16);
                            writetoLCD( 40, 1, dataseq); //send some data
                            memcpy(&dataseq[0], (const uint8 *) "      POLL      ", 16);
                            writetoLCD( 16, 1, dataseq); //send some data
        }
                        else
                        {
                            toggle = 1;
    #if (DR_DISCOVERY == 1)
                            memcpy(&dataseq[0], (const uint8 *) " DISCOVERY MODE ", 16);
    #else
                            memcpy(&dataseq[0], (const uint8 *) " NON DISCOVERY  ", 16);
#endif
                            writetoLCD( 40, 1, dataseq); //send some data
                            sprintf((char*)&dataseq[0], "%llX", instance_get_addr());
                            writetoLCD( 16, 1, dataseq); //send some data
                        }
                    }

                }
                else if(instanceanchorwaiting() == 2)
                {
                    dataseq[0] = 0x2 ;  //return cursor home
                    writetoLCD( 1, 0,  dataseq);
                    memcpy(&dataseq[0], (const uint8 *) "    RANGING WITH", 16);
                    writetoLCD( 40, 1, dataseq); //send some data
                    sprintf((char*)&dataseq[0], "%llX", instance_get_tagaddr());
                    writetoLCD( 16, 1, dataseq); //send some data
                }
            }
        }
#ifdef USB_SUPPORT //this is set in the port.h file
        usb_run();
#endif
    }


    return 0;
}



