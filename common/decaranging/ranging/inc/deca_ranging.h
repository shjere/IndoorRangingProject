#ifndef DECA_RANGING_H_
#define DECA_RANGING_H_

// Includes
#include "deca_device_api.h"
#include "deca_param_types.h"
#include "deca_regs.h"
#include "deca_version.h"

// max range estimates to keep in memory
#define DWR_MAX_RANGE_HISTORY (32)
// Superframe period, governing master beacons
#define DWR_SUPERFRAME_PERIOD_MS        (1000)
// Frames at 10 Hz during which each tag gets position estimate
#define DWR_FRAME_PERIOD_MS             (100)
// Subframes in each frame dedicated to a single tag
#define DWR_SUBFRAME_MAXQUERIES         (5) // (max tags)
#define DWR_SUBFRAME_PERIOD_MS (DWR_FRAME_PERIOD_MS/DWR_SUBFRAME_MAXQUERIES)
#define DWR_SUBFRAME_MAXRESPONSES       (6) // (max anchors)
// Transmission timing and dithering
#define DWR_RX_TIMEOUT_US            (25000)
#define DWR_REPLY_DELAY_US           (500)
#define DWR_REPLY_SLOTPERIOD_US      (2000)
// max random delays
#define DWR_TX_MAXRANDDELAY_US (10000)
// DW High frequency timer units
#define DWR_TIMER_UNITS_HI_NS (4)
#define DWR_TIMER_UNITS_LO_PS (15)
#define DWR_TIMER_HI_31_BITS  (0xFFFFFFFE)
#define DWR_TIMER_MAX_40_BITS (1099511627776)
// Return types
#define DWR_RETURN_OK    (0)
#define DWR_RETURN_ERR   (-1)
// Propagation constants
#define SPEED_OF_LIGHT      (299702547.0)
#define SPEED_OF_LIGHT_CM_PER_PS  (0.02998)
#define SPEED_OF_LIGHT_CM_PER_NS  (29.9800)
#define SPEED_OF_LIGHT_CM_PER_US  (29980.0)
#define DWM1000_ANT_DELAY (32820)
#define DWT_CONST_A_PRF16 (115.72f)
#define DWT_CONST_A_PRF64 (121.74f)
#define POW_2_17 (131072)
// Typical RF constants
#define DWT_PRF_64M_RFDLY   (514.462f)
#define DWT_PRF_16M_RFDLY   (513.9067f)
// Frame byte lengths
#define STANDARD_FRAME_SIZE   (64)
#define FRAME_CRC             (2)
#define FRAME_AND_ADDR_DATA   (9)
#define FRAME_AND_ADDR_BEACON (7)
#define MAX_PAYLOAD_DATA   (STANDARD_FRAME_SIZE - FRAME_AND_ADDR_DATA   - FRAME_CRC)

// Node Type
typedef enum 
{
	// never listen
	DWR_OPT_RXNONE = 0x01,
	// always listen
	DWR_OPT_RXALL = 0x02,
	// listen with timeout
	DWR_OPT_RXTO = 0x04,
	// listen to TWR requests
	DWR_OPT_RSPTWR = 0x08,
	// listen to beacons
	DWR_OPT_RSPBCN = 0x10,
	// send range summaries to gateway
	DWR_OPT_SENDSUM = 0x20,
	// relay range summaries to user
	DWR_OPT_GATEWAY = 0x40
}DWR_NodeOpts;

// Ranging message types -- See DW1000 manual pg. 213
typedef enum
{
	// two way ranging types
	DWR_MSG_RANGE_INIT    = 0x01,
	DWR_MSG_RANGE_RESP    = 0x02,
	DWR_MSG_RANGE_FINAL   = 0x03,
	// information relaying
	DWR_MSG_RANGE_SUMMARY = 0x04,
	// node beacons
	DWR_MSG_BEACON        = 0x05
}DWR_MsgType;

// Ranging states
typedef enum 
{
	DWR_STATE_IDLE        = 0x00,
	DWR_STATE_RANGE_INIT  = 0x01,
	DWR_STATE_RANGE_RESP  = 0x02,
	DWR_STATE_RANGE_FINAL = 0x03,
}DWR_State;

// Certain 802.15.4 frame types
typedef enum
{
	FRAME_BEACON = 0x00,
	FRAME_DATA   = 0x01,
	FRAME_ACK    = 0x02,
	FRAME_MAC    = 0x03,
	FRAME_RES1   = 0x04,
	FRAME_RES2   = 0x05,
	FRAME_RES3   = 0x06,
	FRAME_RES4   = 0x07
}DWR_FrameType_t;

// Debugging signals
typedef enum
{
	DWR_EVENT_ERROR      = 0x00,
	DWR_EVENT_RXGOOD     = 0x01,
	DWR_EVENT_TXGOOD     = 0x02,
	DWR_EVENT_IRQ        = 0x03,
	DWR_EVENT_RXINIT     = 0x04,
	DWR_EVENT_RXRESP     = 0x05,
	DWR_EVENT_RXFIN      = 0x06,
	DWR_EVENT_BADFRAME   = 0x07,
	DWR_EVENT_UNKNOWN_IRQ= 0x08,
	DWR_EVENT_RXSUM      = 0x09,
}DWR_EventType;

// Standard DWR Message Type
#define PAYLOAD_DATA_LEN (53)
typedef struct __attribute__((__packed__))
{
	uint8_t msgType;
	uint8_t rangeSeq;
	uint8_t msgSrcAddr[2];
	uint64_t t_tx;
	uint64_t t_rx;
	uint64_t ts3;
	uint64_t ts4;
	uint64_t ts5;
	uint64_t ts6;
	uint8_t fploss;
} 
DWR_MsgData_t;

// Standard IEEE 802.15.4 Message Types
#define DSSS_FRAME_HEADER_LEN (9)
typedef struct __attribute__((__packed__))
{
	uint8_t frameCtrl[2];                           
	uint8_t seqNum;                               
	uint8_t panId[2];                       
	uint8_t destAddr[2];                    
	uint8_t sourceAddr[2];                        
	uint8_t msgData[MAX_PAYLOAD_DATA] ; 
	uint8_t fcs[2];                             
} ieee_frame_dsss_t ;

// Two-way ranging timing structure
typedef struct
{
	uint16_t srcAddr;
	uint16_t dstAddr;
	uint16_t seq;
	uint64_t tstamp1;
	uint64_t tstamp2;
	uint64_t tstamp3;
	uint64_t tstamp4;
	uint64_t tstamp5;
	uint64_t tstamp6;
	float fppwr;
	float cirp;
	float fploss;
} DWR_TwrTiming_t;

// Range estimation structure
typedef struct
{
	uint8_t nodeOpts;
	uint16_t nodeAddr;
	int rangeEst;
	float pathLoss; 
} DWR_RangeEst_t;

// Beacon reception structure
typedef struct
{
	uint16_t nodeAddr;
	uint16_t seq;
	uint64_t txstamp;
	uint64_t rxstamp;
	uint32_t rssi;
	float fppwr;
	float cirp;
	float fploss;
} DWR_Beacon_t;

// Decaranging initialization options struct
typedef struct
{
	uint16_t panId;
	uint16_t addr;
	DWR_NodeOpts nodeOpts;
	void (*cbRangeComplete)(DWR_TwrTiming_t *timing);
	void (*cbBeaconReceived)(DWR_Beacon_t *beacon);
	int (*spiSendPacket)(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, const uint8_t* body);
	int (*spiReadPacket)(uint16_t headerLen, const uint8_t* header, uint32_t bodyLen, uint8_t* body);
	void (*cbDebug)(uint8_t eventType);
	void (*sleepms)(uint32_t tmsec);
	float (*rng)(void);
} DWR_Config_t;

// ===== Public Function Definitions =====
int decaranging_init( DWR_Config_t config );
int decaranging_sendtwr();
int decaranging_sendbcn();
void decaranging_isr();
uint32_t decaranging_devid();
void decaranging_checkovrf();
uint32_t decaranging_getovrf();
void decaranging_resettxr();

// ===== HW-Specific Functions =====
void decaranging_gpiotoggle_green();
void decaranging_gpiotoggle_red();

#endif // DECARANGING_H_