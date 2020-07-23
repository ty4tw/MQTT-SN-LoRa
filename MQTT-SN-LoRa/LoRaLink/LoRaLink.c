 /**************************************************************************************
 *
 * LoRaLink.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#include <stdbool.h>
#include "LoRaLink.h"
#include "LoRaLinkCrypto.h"
#include "sx1276.h"
#include "timer.h"
#include "delay.h"
#include "LoRaLinkApi.h"
#include "device.h"
#include "utilities.h"

/*!
 * LoRaLink parameters
 */
#define LORALINK_WINDOW_TIMEOUT   6
#define LORALINK_CODERATE         1       //  4/5
#define LORALINK_PREAMBLE_LENGTH  8
#define LORALINK_DUTYCYCLE        10      //  10%

#define LORALINK_RSSI_THRESH     -83
#define LORALINK_MAX_CARRIERSENSE_TIME  5

#define TX_TIMEOUT_DEV_VAL      4000

/*!
 * LBT delay range
 */
#define RND_UPL         (uint32_t)400    // upper limit 400ms
#define RND_LWL         (uint32_t)100    // lower limit 100ms


/*!
 * ARIB T-108 standard Parameters
 */
uint8_t  MaxPayloadDwell0[] = {91, 188, 200, 244, 244, 244, 244, 244 };
uint8_t  MaxPayloadDwell1[] = { 0, 0, 15, 57, 125, 244, 244, 244 };

// 24ch - 38ch
uint32_t FreqenciesDwell0[] = { 920600000, 920800000, 921000000, 921200000, 921400000, 921600000,
						        921800000, 922000000, 922200000, 922400000, 922600000, 922800000, 923000000, 923200000, 923400000 };
// 39ch - 62ch
uint32_t FreqenciesDwell1[] = { 923600000, 923800000, 924000000, 924200000, 924400000, 924600000, 924800000, 925000000, 925200000,
		                        925400000, 925600000, 925800000, 926000000, 926200000, 926400000, 926600000, 926800000, 927000000,
		                        927200000, 927400000, 927600000, 927800000, 928000000 };

uint8_t  DwelltimeRange[]   = { 24, 39, 62 };

uint32_t Bandwidths[]       = { 125000, 125000, 125000, 125000, 125000, 125000, 250000, 0 };



/*
 * Module context.
 */
static LoRaLinkCtx_t LoRaLinkCtx = { 0 };


RadioEvents_t LoRaLinkRadioEvents = { 0 };

LoRaLinkPacket_t LoRaLinkPacket = { 0 };

/*!
 * Structure used to store the radio Tx event data
 */
struct
{
    TimerTime_t CurTime;
}TxDoneParams;

/*!
 * Structure used to store the radio Rx event data
 */
RxDoneParams_t RxDoneParams;

/*!
 * LoRaLink Device State Machine Status
 */
volatile LoRaLinkDeviceStatus_t DeviceStatus;

/*!
 * LoRaLink radio events status
 */
LoRaLinkRadioEvents_t LoRaLinkRadioEventsStatus = { .Value = 0 };

/*!
 * TxData is ready
 */
bool LoRaLinkNextTx = false;


/*
 * Forward declarations
 */
static void ProcessRadioRxDone( void );
static void ProcessRadioTxDone( void );
static void ProcessRadioRxTimeout( void );
static void ProcessRadioTxTimeout( void );
static void ProcessRadioRxError( void );
static void ProcessRadioTxDelaied( void );

static void OnRadioTxDone( void );
static void OnRadioRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void OnRadioTxTimeout( void );
static void OnRadioRxError( void );
static void OnRadioRxTimeout( void );
static void OnTxDelayedTimerEvent( void *context );

static void LoRaLinkHandleIrqEvents( void );
static bool scheduleTx( void );
static bool SetTxConfig( TxConfigParams_t* txConfig, TimerTime_t* txTimeOnAir );
static bool SetRxConfig( RxConfigParams_t* rxConfig );
static void CalcBackOffTime( void );
static uint32_t GetBandwidth( uint8_t sfValue );
static uint8_t GetMaxPayloadLength( uint8_t sfValue );



void LoRaLinkSetPanId(uint16_t panId)
{
	LoRaLinkCtx.LoRaLinkPanId = panId;
}

void LoRaLinkSetDeviceAddr(uint8_t addr)
{
	LoRaLinkCtx.LoRaLinkDeviceAddr = addr;
}

void LoRaLinkInitilize(void)
{
	LoRaLinkRadioEvents.TxDone = OnRadioTxDone;
	LoRaLinkRadioEvents.RxDone = OnRadioRxDone;
	LoRaLinkRadioEvents.RxError = OnRadioRxError;
	LoRaLinkRadioEvents.TxTimeout = OnRadioTxTimeout;
	LoRaLinkRadioEvents.RxTimeout = OnRadioRxTimeout;
	LoRaLinkRadioEvents.FhssChangeChannel = NULL;

	SX1276Init(&LoRaLinkRadioEvents);

	TimerInit(&LoRaLinkCtx.TxDelayedTimer, OnTxDelayedTimerEvent);
	LoRaLinkCtx.LastTxDoneTime = 0;
}

LoRaLinkStatus_t LoRaLinkDeviceInit( uint8_t* key, uint16_t panId, uint8_t devAddr,uint8_t syncWord,  uint8_t uplinkCh, uint8_t dwnlinkCh, LoRaLinkSf_t sfValue, int8_t power )
{
	if ( ( uplinkCh <= DwelltimeRange[0] && uplinkCh < DwelltimeRange[1] ) && ( dwnlinkCh <= DwelltimeRange[0] && dwnlinkCh < DwelltimeRange[1] ) )
	{
		LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_0;
		LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell0[ dwnlinkCh - DwelltimeRange[0] ];
		LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell0[ uplinkCh - DwelltimeRange[0] ];
		LoRaLinkCtx.TxConfig.DutyCycle = 0;
		LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell0[SF_12 - sfValue];
	}
	else if ( ( uplinkCh >= DwelltimeRange[1] && uplinkCh <= DwelltimeRange[2] ) && ( dwnlinkCh >= DwelltimeRange[1] && dwnlinkCh <= DwelltimeRange[2] ) )
	{
		if ( sfValue > SF_10 )
		{
			return LORALINK_STATUS_DATARATE_INVALID;
		}
		else
		{
			LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_1;
			LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell1[ dwnlinkCh - DwelltimeRange[1] ];
			LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell1[ uplinkCh - DwelltimeRange[1] ];
			LoRaLinkCtx.TxConfig.DutyCycle = LORALINK_DUTYCYCLE;
			LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell1[SF_12 - sfValue];
		}
	}
	else
	{
		return LORALINK_STATUS_FREQUENCY_INVALID;
	}

	LoRaLinkCtx.RxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.TxPower = power;
	LoRaLinkCtx.RxConfig.RxContinuous = true;
	LoRaLinkCtx.RxConfig.WindowTimeout = LORALINK_WINDOW_TIMEOUT;

	LoRaLinkCryptoSetKey( key );
	LoRaLinkSetDeviceId( panId, devAddr);

	SX1276SetSyncword(syncWord);

	return LORALINK_STATUS_OK;
}



LoRaLinkStatus_t LoRaLinkUart( uint8_t* key, uint16_t panId, uint8_t devAddr, LoRaLinkUartType_t uartType, uint8_t syncWord, uint8_t uplinkCh, uint8_t dwnlinkCh, LoRaLinkSf_t sfValue, int8_t power )
{
	LoRaLinkStatus_t rc = LORALINK_STATUS_ERROR;

	if ( ( dwnlinkCh >= DwelltimeRange[0] && dwnlinkCh < DwelltimeRange[1] ) && ( uplinkCh >= DwelltimeRange[0] && uplinkCh < DwelltimeRange[1] ) )
	{
		LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_0;
		LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell0[ uplinkCh - DwelltimeRange[0] ];
		LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell0[ dwnlinkCh - DwelltimeRange[0] ];
		LoRaLinkCtx.TxConfig.DutyCycle = 0;
		LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell0[SF_12 - sfValue];
	}
	else if ( ( dwnlinkCh >= DwelltimeRange[1] && dwnlinkCh <= DwelltimeRange[2] ) && ( uplinkCh >= DwelltimeRange[1] && uplinkCh <= DwelltimeRange[2] ) )
	{
		if ( sfValue > SF_10 )
		{
			return LORALINK_STATUS_DATARATE_INVALID;
		}
		else
		{
			LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_1;
			LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell1[ uplinkCh - DwelltimeRange[1] ];
			LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell1[ dwnlinkCh - DwelltimeRange[1] ];
			LoRaLinkCtx.TxConfig.DutyCycle = LORALINK_DUTYCYCLE;
			LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell1[SF_12 - sfValue];
		}
	}
	else
	{
		return LORALINK_STATUS_FREQUENCY_INVALID;
	}

	LoRaLinkCtx.RxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.TxPower = power;
	LoRaLinkCtx.RxConfig.WindowTimeout = LORALINK_WINDOW_TIMEOUT;

	LoRaLinkCryptoSetKey( key );
	LoRaLinkSetDeviceId( panId, devAddr);

	SX1276SetSyncword(syncWord);

	if ( uartType == LORALINK_UART_TX )
	{
		LoRaLinkCtx.RxConfig.RxContinuous = false;
		DeviceStatus = DEVICE_STATE_TX_INIT;
	}
	else
	{
		LoRaLinkCtx.RxConfig.RxContinuous = true;
		DeviceStatus = DEVICE_STATE_RX_INIT;
	}

//	DisableLowPower();

	rc = LORALINK_STATUS_OK;
	LoRaLinkApiReadParameters_t resp = { 0 };
	LoRaLinkApi_t api = { 0 };
	LoRaLinkPacket_t ack = { 0 };

	while ( true )
	{
		LoRaLinkHandleIrqEvents();

		switch ( (int)DeviceStatus )
		{
		case DEVICE_STATE_RX_INIT:
			SetRxConfig( &LoRaLinkCtx.RxConfig );
			DeviceStatus = DEVICE_STATE_RX;
			break;

		case DEVICE_STATE_RX:
			SX1276SetRx(0);
			DeviceStatus = DEVICE_STATE_SLEEP;
			break;

		case DEVICE_STATE_RX_DONE:
			LoRaLinkApiWrite( &LoRaLinkPacket );
			DeviceStatus = DEVICE_STATE_RX_INIT;
			break;

		case DEVICE_STATE_RX_TIMEOUT:
		case DEVICE_STATE_RX_ERROR:
			DeviceStatus = DEVICE_STATE_RX_INIT;
			break;

		case DEVICE_STATE_TX_INIT:
			if ( ( LoRaLinkApiRead( &api, &resp) == true ) && (resp.Available == true) && ( resp.Error == false ) )
			{
				// create TxPacket
				LoRaLinkApiSetTxData( &LoRaLinkCtx.TxMsg, &api );
				LoRaLinkNextTx = true;
				DeviceStatus = DEVICE_STATE_TX;
			}
			break;

		case DEVICE_STATE_TX:
			if ( LoRaLinkNextTx )
			{
				LoRaLinkNextTx = scheduleTx();
			}
			break;

		case DEVICE_STATE_CYCLE:
			TimerSetValue( &LoRaLinkCtx.TxDelayedTimer, LoRaLinkCtx.BackoffTime );
			TimerStart(&LoRaLinkCtx.TxDelayedTimer);
			DeviceStatus = DEVICE_STATE_SLEEP;
			break;

		case DEVICE_STATE_TX_DONE:
			ack.FRMPayloadType = API_RSP_ACK;
			ack.FRMPayloadSize = 0;
			ack.DestAddr = api.SourceAddr;
			ack.SourceAddr = api.SourceAddr;
			ack.Rssi = 0;
			ack.Snr = 0;
			LoRaLinkApiWrite(&ack);
			DeviceStatus = DEVICE_STATE_TX_INIT;
			break;

		case DEVICE_STATE_SLEEP:
			break;

		default:
			break;
		}
	}
	return rc;
}


static LoRaLinkStatus_t LoRaLinkSendPacket( uint32_t timeout )
{
	LoRaLinkNextTx = true;
	uint32_t  nfcTime = 0;
	int32_t   delay;


	while ( true )
	{
		LoRaLinkHandleIrqEvents();

		switch ( (int)DeviceStatus )
		{
		case DEVICE_STATE_TX:
			if ( LoRaLinkNextTx )
			{
				LoRaLinkNextTx = scheduleTx();
			}
			break;

		case DEVICE_STATE_CYCLE:
			TimerSetValue( &LoRaLinkCtx.TxDelayedTimer, LoRaLinkCtx.BackoffTime );
			TimerStart(&LoRaLinkCtx.TxDelayedTimer);
			DeviceStatus = DEVICE_STATE_SLEEP;
			break;

		case DEVICE_STATE_TX_NO_FREE_CH:
			delay = randr( RND_LWL, RND_UPL);

			if ( ( nfcTime += delay ) < timeout )
			{
				TimerSetValue( &LoRaLinkCtx.TxDelayedTimer, delay );
				TimerStart(&LoRaLinkCtx.TxDelayedTimer);
				DeviceStatus = DEVICE_STATE_SLEEP;
			}
			else
			{
				return LORALINK_STATUS_CHANNEL_NOT_FREE;
			}
			break;

		case DEVICE_STATE_SLEEP:
			DeviceLowPowerHandler( );
			break;

		case DEVICE_STATE_TX_DONE:
			return LORALINK_STATUS_OK;

		case DEVICE_STATE_TX_TIMEOUT:
			return LORALINK_STATUS_TX_TIMEOUT;

		default:
			return LORALINK_STATUS_ERROR;
		}
	}
}

LoRaLinkStatus_t LoRaLinkSend( uint8_t destAddr, uint8_t payloadType, uint8_t* buffer, uint8_t buffLen, uint32_t timeout )
{
	LoRaLinkApi_t api = { 0 };
	LoRaLinkPacket_t pkt = { 0 };

	if ( buffLen > LoRaLinkGetMaxPayloadLength() )
	{
		return LORALINK_STATUS_LENGTH_ERROR;
	}

	api.PanId = LoRaLinkCtx.LoRaLinkPanId;
	api.DestinationAddr = destAddr;
	api.SourceAddr = LoRaLinkCtx.LoRaLinkDeviceAddr;
	api.PayloadType = payloadType;
	api.PayloadLen = buffLen;
	memcpy1( api.Payload, buffer, buffLen );

	LoRaLinkApiSetTxData( &pkt, &api );
	DeviceStatus = DEVICE_STATE_TX;

	return LoRaLinkSendPacket(timeout);
}

LoRaLinkStatus_t LoRaLinkRecvPacket( LoRaLinkPacket_t* pkt, uint32_t timeout )
{
	DeviceStatus = DEVICE_STATE_RX_INIT;

	while ( true )
	{
		LoRaLinkHandleIrqEvents();

		switch ( (int)DeviceStatus )
		{
		case DEVICE_STATE_RX_INIT:
			SetRxConfig( &LoRaLinkCtx.RxConfig );
			DeviceStatus = DEVICE_STATE_RX;
			break;

		case DEVICE_STATE_RX:
			SX1276SetRx( timeout );

			if ( timeout > 0 )
			{
				DeviceStatus = DEVICE_STATE_SLEEP;
			}
			break;

		case DEVICE_STATE_SLEEP:
			DeviceLowPowerHandler( );

			break;

		case DEVICE_STATE_RX_DONE:
			pkt->Rssi = LoRaLinkPacket.Rssi;
			pkt->Snr = LoRaLinkPacket.Snr;
			pkt->PanId = LoRaLinkPacket.PanId;
			pkt->SourceAddr = LoRaLinkPacket.SourceAddr;
			pkt->FRMPayloadType = LoRaLinkPacket.FRMPayloadType;
			pkt->FRMPayloadSize = LoRaLinkPacket.FRMPayloadSize;
			pkt->FRMPayload = LoRaLinkPacket.FRMPayload;
			return LORALINK_STATUS_OK;

		case DEVICE_STATE_RX_TIMEOUT:
			return LORALINK_STATUS_RX_TIMEOUT;

		default:
			return LORALINK_STATUS_ERROR;
			break;
		}
	}
}

void LoRaLinkSetTxData( LoRaLinkPacket_t* pkt )
{
	// encrypt Payload and Set it into LoRaLinkCtx.PktBuffer
	pkt->Buffer = LoRaLinkCtx.PktBuffer;
	LoRaLinkCryptoSecureMessage(pkt);
	LoRaLinkSerializeData(pkt);
	LoRaLinkCtx.PktBufferLen = pkt->BufSize;
}

LoRaLinkStatus_t LoRaLinkSerializeData( LoRaLinkPacket_t* pkt)
{
	if ( pkt == NULL )
	{
		return LORALINK_STATUS_PARAMETER_INVALID;
	}
	if ( pkt->Buffer == NULL )
	{
		return LORALINK_STATUS_PARAMETER_INVALID;
	}

	uint8_t* pos = pkt->Buffer;

	*pos++ = pkt->PanId >> 8;
	*pos++ = (uint8_t)pkt->PanId & 0xff;
	*pos++ = pkt->DestAddr;
	*pos++ = pkt->SourceAddr;
	*pos++ = pkt->FRMPayloadType;

	memcpy1(pos, pkt->FRMPayload, pkt->FRMPayloadSize );
	pos += pkt->FRMPayloadSize;
	setUint32(pos, pkt->MIC);
	pkt->BufSize = pkt->FRMPayloadSize + LORALINK_HDR_LEN + LORALINK_MIC_LEN;

	return LORALINK_STATUS_OK;
}

LoRaLinkStatus_t LoRaLinkDeserializeData(LoRaLinkPacket_t* pkt, RxDoneParams_t* rxData)
{
	if ( pkt == NULL || rxData == NULL )
	{
		return LORALINK_STATUS_PARAMETER_INVALID;
	}

	uint8_t* pos = rxData->Payload;

	setUint16(pos, pkt->PanId);
	pos += 2;
	pkt->DestAddr = *pos++;
	pkt->SourceAddr = *pos++;
	pkt->FRMPayloadType = *pos++;
	pkt->FRMPayloadSize = rxData->Size - LORALINK_HDR_LEN;
	memcpy1(pkt->FRMPayload, pos, pkt->FRMPayloadSize );
	setUint32(pos + pkt->FRMPayloadSize, pkt->MIC);

	return LORALINK_STATUS_OK;
}

LoRaLinkStatus_t LoRaLinkSetDeviceId( uint16_t panId, uint8_t devAddr )
{
	LoRaLinkStatus_t rc = LORALINK_STATUS_PARAMETER_INVALID;

	if ( panId > 0 && devAddr > 0 )
	{
		LoRaLinkCtx.LoRaLinkPanId = panId;
		LoRaLinkCtx.LoRaLinkDeviceAddr = devAddr;
		rc = LORALINK_STATUS_OK;
	}
	return rc;
}

static bool scheduleTx( void )
{
	CalcBackOffTime();

	if ( LoRaLinkCtx.BackoffTime > 0 )
	{
		DeviceStatus = DEVICE_STATE_CYCLE;
	}
	else if ( SX1276IsChannelFree( MODEM_FSK, LoRaLinkCtx.TxConfig.Frequency, LORALINK_RSSI_THRESH, LORALINK_MAX_CARRIERSENSE_TIME ) == true )
	{
		SetTxConfig( &LoRaLinkCtx.TxConfig, &LoRaLinkCtx.TxTimeOnAir );
		SX1276Send( LoRaLinkCtx.PktBuffer,  LoRaLinkCtx.PktBufferLen );
		return false;
	}
	else
	{
		DeviceStatus = DEVICE_STATE_TX_NO_FREE_CH;
	}
	return true;
}


static bool SetRxConfig( RxConfigParams_t* rxConfig )
{
    RadioModems_t modem = MODEM_LORA;
    uint8_t maxPayload = 0;
    uint32_t bandwidth = GetBandwidth(rxConfig->SFValue);

    if( SX1276GetStatus( ) != RF_IDLE )
    {
    	DLOG_MSG_INT( "RadioState", (int)SX1276GetStatus( ) );
        return false;
    }

    SX1276SetChannel( rxConfig->Frequency );

	SX1276SetRxConfig( modem, bandwidth, (uint32_t)rxConfig->SFValue, LORALINK_CODERATE, 0, LORALINK_PREAMBLE_LENGTH, rxConfig->WindowTimeout, false, 0, true, false, 0, false, rxConfig->RxContinuous );

	maxPayload = GetMaxPayloadLength(rxConfig->SFValue);

    SX1276SetMaxPayloadLength( modem, maxPayload + LORALINK_HDR_LEN + LORALINK_MIC_LEN);

    return true;
}


static bool SetTxConfig( TxConfigParams_t* txConfig, TimerTime_t* txTimeOnAir )
{
    RadioModems_t modem = MODEM_LORA;
    uint32_t bandwidth = GetBandwidth(txConfig->SFValue);

    // Setup the radio frequency
    SX1276SetChannel( txConfig->Frequency );

	SX1276SetTxConfig( modem, txConfig->TxPower, 0, bandwidth, txConfig->SFValue, LORALINK_CODERATE , LORALINK_PREAMBLE_LENGTH, false, true, 0, 0, false, TX_TIMEOUT_DEV_VAL );

    // Setup maximum payload length of the radio driver
    SX1276SetMaxPayloadLength( modem, LoRaLinkCtx.PktBufferLen );
    // Get the time-on-air of the next tx frame
    *txTimeOnAir = SX1276GetTimeOnAir( modem, LoRaLinkCtx.PktBufferLen );

    return true;
}

LoRaLinkPacket_t* LoRaLinkClearPacket( LoRaLinkPacket_t* pkt )
{
	pkt->BufSize = 0;
	pkt->Buffer = NULL;
	pkt->DestAddr = 0;
	pkt->FRMPayload = NULL;
	pkt->FRMPayloadSize = 0;
	pkt->FRMPayloadType = 0;
	pkt->MIC = 0;
	pkt->PanId = 0;
	pkt->Rssi = 0;
	pkt->Snr = 0;
	pkt->SourceAddr = 0;
	return pkt;
}

uint8_t LoRaLinkGetSourceAddr(void)
{
	return LoRaLinkCtx.LoRaLinkDeviceAddr;
}

uint16_t LoRaLinkGetPanId(void)
{
	return LoRaLinkCtx.LoRaLinkPanId;
}

uint8_t LoRaLinkGetMaxPayloadLength(void)
{
	return GetMaxPayloadLength( LoRaLinkCtx.TxConfig.SFValue );
}

static uint8_t GetMaxPayloadLength( uint8_t sfValue )
{
	if ( LoRaLinkCtx.LoRaLinkDwelltime == DWELLTIME_0 )
	    {
	    	return MaxPayloadDwell0[ SF_12 - sfValue ];
	    }
	    else
	    {
	    	return MaxPayloadDwell1[ SF_12 - sfValue ];
	    }
}

static uint32_t GetBandwidth( uint8_t sfValue )
{
    switch( Bandwidths[SF_12 - sfValue] )
    {
        default:
        case 125000:
            return 0;
        case 250000:
            return 1;
        case 500000:
            return 2;
    }
}

static void CalcBackOffTime( void )
{
	if( LoRaLinkCtx.TxConfig.DutyCycle > 0 )
	{
		TimerTime_t elapsed = TimerGetElapsedTime(LoRaLinkCtx.LastTxDoneTime);
		TimerTime_t backoff = LoRaLinkCtx.TxTimeOnAir * 100 / LoRaLinkCtx.TxConfig.DutyCycle - LoRaLinkCtx.TxTimeOnAir;

		if ( elapsed >= backoff || elapsed == 0 )
		{
			LoRaLinkCtx.BackoffTime = 0;
		}
		else
		{
			LoRaLinkCtx.BackoffTime = backoff - elapsed;
		}
	}
	else
	{
		LoRaLinkCtx.BackoffTime = 0;
	}
}


static void LoRaLinkHandleIrqEvents( void )
{
	LoRaLinkRadioEvents_t events;

    CRITICAL_SECTION_BEGIN( );
    events = LoRaLinkRadioEventsStatus;
    LoRaLinkRadioEventsStatus.Value = 0;
    CRITICAL_SECTION_END( );

    if( events.Value != 0 )
    {
        if( events.Events.TxDone == 1 )
        {
        	ProcessRadioTxDone( );
        }
        if( events.Events.RxDone == 1 )
        {
            ProcessRadioRxDone( );
        }
        if( events.Events.TxTimeout == 1 )
        {
            ProcessRadioTxTimeout( );
        }
        if( events.Events.RxError == 1 )
        {
        	ProcessRadioRxError( );
        }
        if( events.Events.RxTimeout == 1 )
        {
            ProcessRadioRxTimeout( );
        }
        if ( events.Events.TxDelaied == 1 )
        {
        	ProcessRadioTxDelaied( );
        }
    }
}


/*
 * Interrupt callbacks for Modem
 */
static void ProcessRadioTxDone( void )
{
	SX1276SetSleep();
	LoRaLinkCtx.LastTxDoneTime = TxDoneParams.CurTime;
	DeviceStatus = DEVICE_STATE_TX_DONE;

}

static void ProcessRadioRxDone( void )
{
	SX1276SetSleep();
	LoRaLinkApiGetRxData( &LoRaLinkPacket, &RxDoneParams );
	if ( ( LoRaLinkPacket.PanId == LoRaLinkCtx.LoRaLinkPanId ) &&
	   ( ( LoRaLinkPacket.DestAddr == LoRaLinkCtx.LoRaLinkDeviceAddr) || ( LoRaLinkPacket.DestAddr == LORALINK_MULTICAST_ADDR ) ) )
	   {
		if ( LoRaLinkCryptoUnsecureMessage(&LoRaLinkPacket) == LORALINK_CRYPTO_SUCCESS )
		{
			DeviceStatus = DEVICE_STATE_RX_DONE;
			return;
		}
	}
	DeviceStatus = DEVICE_STATE_RX_INIT;
}

static void ProcessRadioRxTimeout( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_RX_TIMEOUT;
}

static void ProcessRadioTxTimeout( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_TX_TIMEOUT;
}

static void ProcessRadioRxError( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_RX_ERROR;
}

static void ProcessRadioTxDelaied( void )
{
	SX1276SetSleep();
	LoRaLinkCtx.BackoffTime = 0;
	DeviceStatus = DEVICE_STATE_TX;
}


/*
 * Radio Interrupt callbacks
 */
static void OnRadioTxDone( void )
{
    TxDoneParams.CurTime = TimerGetCurrentTime( );
    LoRaLinkCtx.LastTxSysTime = SysTimeGet( );

    LoRaLinkRadioEventsStatus.Events.TxDone = 1;

    if( LoRaLinkCtx.LinkProcessNotify != NULL )
    {
        LoRaLinkCtx.LinkProcessNotify( );
    }
}

static void OnRadioRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    RxDoneParams.LastRxDone = TimerGetCurrentTime( );
    RxDoneParams.Payload = payload;
    RxDoneParams.Size = size;
    RxDoneParams.Rssi = rssi;
    RxDoneParams.Snr = snr;

    LoRaLinkRadioEventsStatus.Events.RxDone = 1;

    if( LoRaLinkCtx.LinkProcessNotify != NULL )
    {
        LoRaLinkCtx.LinkProcessNotify( );
    }
}

static void OnRadioTxTimeout( void )
{
	LoRaLinkRadioEventsStatus.Events.TxTimeout = 1;

    if( LoRaLinkCtx.LinkProcessNotify != NULL )
    {
        LoRaLinkCtx.LinkProcessNotify( );
    }
}

static void OnRadioRxError( void )
{
	LoRaLinkRadioEventsStatus.Events.RxError = 1;

    if( LoRaLinkCtx.LinkProcessNotify != NULL )
    {
        LoRaLinkCtx.LinkProcessNotify( );
    }
}

static void OnRadioRxTimeout( void )
{
	LoRaLinkRadioEventsStatus.Events.RxTimeout = 1;

    if( LoRaLinkCtx.LinkProcessNotify != NULL )
    {
        LoRaLinkCtx.LinkProcessNotify( );
    }
}

static void OnTxDelayedTimerEvent( void *context )
{
//	LoRaLinkCtx.BackoffTime = 0;
//	DeviceStatus = DEVICE_STATE_TX;

	LoRaLinkRadioEventsStatus.Events.TxDelaied = 1;

    if( LoRaLinkCtx.LinkProcessNotify != NULL )
    {
        LoRaLinkCtx.LinkProcessNotify( );
    }
}
