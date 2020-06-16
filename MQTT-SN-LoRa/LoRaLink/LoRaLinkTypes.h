 /**************************************************************************************
 *
 * LoRaLinkTypes.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#ifndef LORALINKTYPES_H_
#define LORALINKTYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "systime.h"
#include "timer.h"
#include "radio.h"

#define LORA_SYNCWORD      0x55

typedef enum
{
	API_RX_DATA = 0x80,
	API_TX_DATA = 0x40,
	API_TX_RESP = 0x42,
}LoRaLinkApiType_t;

/*!
 * LoRaLink Device Type
 */
typedef enum
{
	LORALINK_DEVICE,
	LORALINK_MODEM_TX,
	LORALINK_MODEM_RX,
}LoRaLinkDeviceType_t;

/*!
 * LoRaLink Spreading Factor
 */
typedef enum
{
	SF_7 = 7,
	SF_8,
	SF_9,
	SF_10,
	SF_11,
	SF_12,
} LoRaLinkSf_t;

/*!
 * LoRaLink Dwelltime enumration
 */
typedef enum
{
	DWELLTIME_0,
	DWELLTIME_1,
} LoRaLinkDwelltime_t;

/*!
 * LoRaLink State machine status
 */
typedef enum
{
	DEVICE_STATE_RX_INIT,
	DEVICE_STATE_RX,
	DEVICE_STATE_TX_INIT,
	DEVICE_STATE_TX,
	DEVICE_STATE_TX_DONE,
	DEVICE_STATE_RX_DONE,
	DEVICE_STATE_RX_TIMEOUT,
	DEVICE_STATE_TX_TIMEOUT,
	DEVICE_STATE_CYCLE,
	DEVICE_STATE_SLEEP,
	DEVICE_STATE_RX_ERROR,
} LoRaLinkDeviceStatus_t;

/*!
 * LoRaLink receive window enumeration
 */
/*
typedef enum
{
	RX_SLOT_WIN,
	RX_SLOT_WIN_CONTINUOUS,
	RX_SLOT_NONE,
} LoRaLinkRxSlot_t;
*/

/*!
 * Parameter structure for the function SetRxConfig.
 */
typedef struct
{
	/*!
	 * The RX frequency.
	 */
	uint32_t Frequency;
	/*!
	 * RX Spreading Factor.
	 */
	int8_t SFValue;
	/*!
	 * RX bandwidth.
	 */
	uint8_t Bandwidth;
    /*!
     * Set to true, if RX should be continuous.
     */
    bool RxContinuous;
	/*!
	 * RX window timeout
	 */
	uint32_t WindowTimeout;
	/*!
	 * Sets the RX window.
	 */
//	LoRaLinkRxSlot_t RxSlot;
} RxConfigParams_t;

/*!
 * Maximum PHY layer payload size
 */
#define LORA_PHY_MAXPAYLOAD                      255

/*!
 * Parameter structure for the function SetTxConfig.
 */
typedef struct
{
	/*!
	 * Duty cycle Flag.
	 */
	int8_t DutyCycle;
	/*!
	 * The TX frequency.
	 */
	uint32_t Frequency;
	/*!
	 * The TX datarate.
	 */
	int8_t SFValue;
	/*!
	 * The TX power.
	 */
	int8_t TxPower;
	/*!
	 * Frame length to setup.
	 */
	uint16_t PktLen;
} TxConfigParams_t;

/*!
 * Structure used to store the radio Rx event data
 */
typedef struct
{
    TimerTime_t LastRxDone;
    uint8_t *Payload;
    uint16_t Size;
    int16_t Rssi;
    int8_t Snr;
} RxDoneParams_t;

/*!
 * LoRaLink Packet format
 */
//#define LORALINK_PACKET_OVERHEAD  (4)   // PanId + DestAddr + SourceAddr
#define LORALINK_MIC_FIELD_SIZE     (4)
#define LORALINK_MAX_API_LEN     ( LORA_PHY_MAXPAYLOAD + 5 )  // ApiType(1) + Rssi(2) + Snr(2)

#define LORALINK_HDR_LEN  (4)
#define LORALINK_MIC_LEN  (4)

typedef struct
{
	/*!
	 * Serialized message buffer
	 */
	uint8_t* Buffer;
	/*!
	 * Size of serialized message buffer
	 */
	uint8_t BufSize;
	/*!
	 * API Type
	 */
	uint8_t ApiType;
	/*!
	 * PAN ID
	 */
	uint16_t PanId;
	/*!
	 * Destination
	 */
	uint8_t DestAddr;
	/*!
	 * Source
	 */
	uint8_t SourceAddr;
	/*!
	 * RSSI
	 */
	uint16_t Rssi;
	/*!
	 * SNR
	 */
	uint16_t Snr;
	/*!
	 * Frame payload
	 */
	uint8_t* FRMPayload;
	/*!
	 * Size of frame payload
	 */
	uint8_t FRMPayloadSize;
	/*!
	 * Message integrity code (MIC)
	 */
	uint32_t MIC;
} LoRaLinkPacket_t;

typedef struct
{
	/*
	 * Length of packet in PktBuffer
	 */
	uint16_t PktBufferLen;
	/*
	 * Buffer containing the data to be sent or received.
	 */
	uint8_t PktBuffer[LORALINK_MAX_API_LEN];
	/*!
	 * Current processed transmit message
	 */
	LoRaLinkPacket_t TxMsg;
	/*
	 * Buffer containing the upper layer data.
	 */
	uint8_t RxPayload[LORA_PHY_MAXPAYLOAD];

	SysTime_t LastTxSysTime;
	/*
	 * LoRaLink internal state
	 */
	uint32_t LinkState;
	/*
	 * LoRaLink upper layer event functions
	 */
//	LoRaLinkPrimitives_t* LinkPrimitives;
	/*
	 * LoRaLink upper layer callback functions
	 *\warning  Runs in a IRQ context. Should only change variables state.
	 */
	void (*LinkProcessNotify)(void);
	/*
	 * Radio events function pointer
	 */
	RadioEvents_t RadioEvents;
	/*
	 * LoRaLink duty cycle delayed Tx timer
	 */
	TimerEvent_t TxDelayedTimer;
	/*
	 * LoRaLink reception windows timers
	 */
	TimerEvent_t RxWindowTimer;
	/*
	 * LoRaLink Rx configuration
	 */
	RxConfigParams_t RxConfig;
	/*
	 * LoRaLink Tx configuration
	 */
	TxConfigParams_t TxConfig;
	/*
	 * Last transmission time on air
	 */
	TimerTime_t TxTimeOnAir;

	TimerTime_t LastTxDoneTime;

	TimerTime_t BackoffTime;
	/*!
	 * Dwelltime
	 */
	LoRaLinkDwelltime_t LoRaLinkDwelltime;
	/*!
	 * PAN ID
	 */
	uint16_t LoRaLinkPanId;
	/*!
	 * Device Address
	 */
	uint8_t LoRaLinkDeviceAddr;

} LoRaLinkCtx_t;

/*!
 * LoRaLink Status
 */
typedef enum
{
    /*!
     * Service started successfully
     */
    LORALINK_STATUS_OK,
    /*!
     * Service not started - LoRaMAC is busy
     */
    LORALINK_STATUS_BUSY,
    /*!
     * Service not started - invalid parameter
     */
    LORALINK_STATUS_PARAMETER_INVALID,
    /*!
     * Service not started - invalid frequency
     */
    LORALINK_STATUS_FREQUENCY_INVALID,
    /*!
     * Service not started - invalid datarate
     */
    LORALINK_STATUS_DATARATE_INVALID,
    /*!
     * Service not started - invalid frequency and datarate
     */
    LORALINK_STATUS_FREQ_AND_DR_INVALID,
    /*!
     * Service not started - payload length error
     */
    LORALINK_STATUS_LENGTH_ERROR,
    /*!
     *
     */
    LORALINK_STATUS_CHANNEL_NOT_FREE,
	/*!
	 *
	 */
	LORALINK_STATUS_RX_TIMEOUT,
    /*!
     * An error in the cryptographic module is occurred
     */
    LORALINK_STATUS_CRYPTO_ERROR,
	/*!
     * Undefined error occurred
     */
    LORALINK_STATUS_ERROR
}LoRaLinkStatus_t;

/*!
 * LoRaLink Serialized API
 */
typedef struct
{
	uint8_t   ApiType;
	uint16_t  PanId;
	uint8_t   DestinationAddr;
	uint8_t   SourceAddr;
	uint8_t   Payload[LORA_PHY_MAXPAYLOAD];
	uint16_t  PayloadLen;
}LoRaLinkApi_t;




#endif /* LORALINKTYPES_H_ */
