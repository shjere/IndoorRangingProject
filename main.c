/* Includes ------------------------------------------------------------------*/
//#include "stm32f4x7_eth.h"
//#include "netconf.h"
#include "main.h"
//#include "ntb_tcpserver.h"
#include <ntb_hwctrl.h>
//#include "lwip/tcp.h"
#include "deca_ranging.h"
#include "deca_debug.h"
#include "deca_rng.h"
#include "deca_spi.h"
//#include "lwio.h"

// Private variables: TIM3 Update Counter
__IO uint16_t CCR1_Val = 40960;

#define SYSTEMTICK_PERIOD_MS  (10)
#define DWRANGE_POLL_PERIOD_MS (500)
#define MAX_TCP_CLIENTS (4)
#define MAX_DITHER_MS (10)


// TWR broadcasting sequencing
// uid = index, trigger = element
#define MAX_TWR_NODES (8)

// Timing variables
volatile uint32_t localtime_ms = 0;
uint32_t last_1hz_time = 0;
uint32_t last_10hz_time = 0;
uint32_t last_100hz_time = 0;


// Local node identifier
uint16_t local_uid = 9;
 
// Operating mode
//uint16_t operating_mode = 0;

// local DW1000 beacon and twr rate
uint16_t beacon_delay_ms = 0;
uint32_t last_beacon_time = 0;
uint16_t twr_delay_ms = 0;
uint32_t last_twr_time = 0;
int random_dither = 0;
uint8_t twr_queued = 0;


// ===== Public Functions (main.h) ====

void SetBeaconDelay(uint16_t delay_ms)
{
  beacon_delay_ms = delay_ms;
}

void SetTwrDelay(uint16_t delay_ms)
{
  twr_delay_ms = delay_ms;
}

void sendTwrOneShot()
{
  decaranging_sendtwr();
}


void dw_range_complete(DWR_TwrTiming_t *timeStats)
{
  // construct ascii response
  uint8_t msg_ascii[128];
  int msglen = 0;
  uint64_t msg[14];
  msg[0] = timeStats->dstAddr;
  // msg type = 0
  msg[1] = 0;
  // node source address
  msg[2] = timeStats->srcAddr;
  // range sequence
  msg[3] = timeStats->seq;
  // timestamps 1 -> 6
  msg[4] = timeStats->tstamp1;
  msg[5] = timeStats->tstamp2;
  msg[6] = timeStats->tstamp3;
  msg[7] = timeStats->tstamp4;
  msg[8] = timeStats->tstamp5;
  msg[9] = timeStats->tstamp6;
  // rx quality numbers
  msg[10] = -100*timeStats->fppwr;
  msg[11] = timeStats->cirp;
  msg[12] = -100*timeStats->fploss;
  msg[13] = (uint64_t)(decaranging_getovrf());
  // convert to ascii CSV list
  msglen = uintlist2str(msg_ascii, msg, 14);

  // send to all subscribed tcp clients
  //ntb_bcast_tcp(msg_ascii, msglen, NTB_TCPOPT_STREAM_RANGE );

  // blink the bright blue LED to indicate a range complete measurement
  //ntb_led_toggle(NTB_LED_BRIGHT_ORANGE);
}



/*
// TODO: this is currently called from an ISR--it should be put in an outgoing
// queue and serviced opportunistically.
void dw_range_complete(DWR_RangeEst_t *range)
{
  // ignore improbable ranges
  if( range->rangeEst < 0 || range->rangeEst > 5000)
    return;

  // construct ascii response
  uint8_t msg_ascii[64];
  int msglen = 0;
  int msg[5];
  msg[0] = local_uid;
  // range type = 0
  msg[1] = 0;
  msg[2] = range->nodeAddr;
  msg[3] = (int)(range->rangeEst);
  msg[4] = (int)(range->pathLoss*100);
  msglen = intlist2str(msg_ascii, msg, 5);

  // send to all subscribed tcp clients
  ntb_bcast_tcp(msg_ascii, msglen, NTB_TCPOPT_STREAM_RANGE );

  // blink the bright blue LED to indicate a range complete measurement
  ntb_led_toggle(NTB_LED_BRIGHT_ORANGE);
}
*/


void dw_beacon_received(DWR_Beacon_t *beacon)
{
  // construct ascii response
  uint8_t msg_ascii[64];
  int msglen = 0;
  int msg[17];
  msg[0] = local_uid;
  // beacon type = 1
  msg[1] = 1;
  // source
  msg[2] = beacon->nodeAddr;
  // range sequence (1 byte)
  msg[3] = beacon->seq;
  // tx bytes (x5)
  msg[4] = (beacon->txstamp>>32) & 0xFF;
  msg[5] = (beacon->txstamp>>24) & 0xFF;
  msg[6] = (beacon->txstamp>>16) & 0xFF;
  msg[7] = (beacon->txstamp>>8) & 0xFF;
  msg[8] = (beacon->txstamp>>0) & 0xFF;
  // rx bytes (x5)
  msg[9] = (beacon->rxstamp>>32) & 0xFF;
  msg[10] = (beacon->rxstamp>>24) & 0xFF;
  msg[11] = (beacon->rxstamp>>16) & 0xFF;
  msg[12] = (beacon->rxstamp>>8) & 0xFF;
  msg[13] = (beacon->rxstamp>>0) & 0xFF;
  // fppwr
  msg[14] = (int)(beacon->fppwr*100);
  // cirp
  msg[15] = (int)(beacon->cirp);
  // fploss
  msg[16] = (int)(beacon->fploss*100);

  msglen = intlist2str(msg_ascii, msg, 17);

  // send to all subscribed tcp clients
  //ntb_bcast_tcp(msg_ascii, msglen, NTB_TCPOPT_STREAM_RANGE );

  // blink the bright red LED to indicate a beacon has been received
  //ntb_led_toggle(NTB_LED_BRIGHT_ORANGE);
}


/*
int genRandomDither(int maxOfst)
{
  // pick random int between 0 and 2*maxOfst
  int tmp = (int)(dw_rng_float()*2*maxOfst);
  // return a number between -maxOfst and +maxOfst
  return tmp - maxOfst;
}
*/

// Set up a interrupt-driven timer
/*
void TIM_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  uint16_t PrescalerValue = 0;

  // TIM3 Clock Enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  // GPIOB Clock Enable
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

  // GPIOB Configuration: TIM3 CH1 (PB4)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
  GPIO_Init(GPIOB, &GPIO_InitStructure); 

  // Connect TIM Channels to AF
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_2);

  // Enable the TIM3 Global Interrupt
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  // Time base configuration 
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  // TIM3 IT Enable
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  // TIM3 Enable counter
  TIM_Cmd(TIM3, ENABLE);
}
*/

int main()
{
  // Set vector table offset immediately (used for IAP)
  //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x20000);

  
  // initialize NTB tag hardware
  ntb_init_hw();
  //TIM_Config();
       
  //ntb_led_on(NTB_LED_ARM_RED);

  // Read UID value and set mac accordingly
  //local_uid = 9; // Comment out for debugging 
  //MAC_ADDR5 = local_uid;

  // read operating mode
  //operating_mode = ntb_mode_read();

  // Reset ethernet switch
  //ntb_ethswitch_rst();

  // init power measurement
  //ntb_init_pmeas();

  // set ethernet switch to transparent clock mode
  //ntb_ethswitch_set_tclk();

  // decaranging options: always listen, respond to beacons, respond to TWR init
  DWR_NodeOpts nodeOpts = DWR_OPT_RXALL | DWR_OPT_RSPTWR;
  
  // Debugging:
  //DWR_NodeOpts nodeOpts = DWR_OPT_RSPTWR;
  
  
  if( local_uid == 0 )
    nodeOpts |= DWR_OPT_GATEWAY;
  

  // initialize DecaWave ranging
  // configure ranging library
  
  DWR_Config_t dwr_config = {
    .panId = 0xAE70,
    .addr = local_uid,
    .nodeOpts = nodeOpts,
    .spiSendPacket = dw_spi_sendpacket,
    .spiReadPacket = dw_spi_readpacket,
    .sleepms = dw_sleep_msec,
    .rng = dw_rng_float,
    .cbRangeComplete = dw_range_complete,
    .cbBeaconReceived = dw_beacon_received,
    .cbDebug = dw_debug_handler
  };
  

  // random number gen.
  //dw_rng_init();
  

  //int dw_status = decaranging_init( dwr_config );

  //led_toggle_fast();

  /*
  if( dw_status == DWR_RETURN_ERR )
    led_toggle_fast();
  */

  // DWR done initializing. Increase SPI speed
  dw_spi_configprescaler(SPI_BaudRatePrescaler_4);

  
  // Code section for debugging
   
  while(1)
  {

      
      led_on();
      ntb_busywait_ms(100);
      
      //decaranging_sendtwr();
      led_off();
      ntb_busywait_ms(100);
      
      //GPIO_ResetBits(GPIOA, GPIO_Pin_8);    // Set PA8 (Chip Select) low
    
      // Transmit some byte of data
      //SPISend((uint8_t)0b10111010);       // Custom function

      // Std Periph Library function
      //SPI_SendData8(SPI1, 0b10101010);      

      
      // Turn on LED
      //led_on();
      
      // Wait for a certain milliseconds
      //ntb_busywait_ms(100);

      // Turn off LED
      //led_off();
      
      //ntb_busywait_ms(100);

      //decaranging_sendtwr();
      // Wait for some more milliseconds
      //ntb_busywait_ms(10);
    
      //SPISend(0x00);        // Transmit dummy byte and receive data
      
      //GPIO_SetBits(GPIOA, GPIO_Pin_8);    // Set PA8 (Chip Select) high   
  }
  

  // turn off red led and turn on blue to show we're done with init
  //ntb_led_off(NTB_LED_ARM_RED);
  //ntb_led_on(NTB_LED_ARM_BLUE);

  // initialize the random dither for beacon TX
  //random_dither = genRandomDither(MAX_DITHER_MS);
   
   
  
}


void Time_Update(void)
{
  localtime_ms += SYSTEMTICK_PERIOD_MS;
}


#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
  // User can add his own implementation to report the file name and line number,
    // ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) 

  // Infinite loop 
  while (1)
  {}
}
#endif

