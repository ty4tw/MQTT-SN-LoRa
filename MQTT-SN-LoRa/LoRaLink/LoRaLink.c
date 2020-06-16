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
#include "sx1276/sx1276.h"
#include "timer.h"
#include "delay.h"
#include "LoRaLinkApi.h"
#include "device.h"
#include "utilities.h"


#define LORALINK_WINDOW_TIMEOUT   6
#define LORALINK_CODERATE         1       //  4/5
#define LORALINK_PREAMBLE_LENGTH  8
#define LORALINK_DUTYCYCLE        10      //  10%

#define LORALINK_RSSI_THRESH     -85
#define LORALINK_MAX_CARRIERSENSE_TIME  5

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
 * Defines the LoRaMac radio events status
 */
typedef union uLoRaMacRadioEvents
{
    uint32_t Value;
    struct sEvents
    {
        uint32_t RxTimeout : 1;
        uint32_t RxError   : 1;
        uint32_t TxTimeout : 1;
        uint32_t RxDone    : 1;
        uint32_t TxDone    : 1;
    }Events;
}LoRaLinkRadioEvents_t;

uint32_t Bandwidths[] = { 125000, 125000, 125000, 125000, 125000, 125000, 250000, 0 };
uint8_t MaxPayloadDwell0[] = { 54, 54, 54, 118, 245, 245, 245, 245 };
uint8_t MaxPayloadDwell1[] = { 0, 0, 14, 56, 128, 245, 245, 245 };

uint32_t FreqenciesDwell0[] = { 920600000, 920800000, 921000000, 921200000, 921400000, 921600000,
						        921800000, 922000000, 922200000, 922400000, 922600000, 922800000, 923000000, 923200000, 923400000 };

uint32_t FreqenciesDwell1[] = { 923600000, 923800000, 924000000, 924200000, 924400000, 924600000, 924800000, 925000000, 925200000,
		                        925400000, 925600000, 925800000, 926000000, 926200000, 926400000, 926600000, 926800000, 927000000,
		                        927200000, 927400000, 927600000, 927800000 };

uint8_t DwelltimeRange[] = { 24, 39, 61 };



/*!
 * LoRaLink Device State Machine Status
 */
LoRaLinkDeviceStatus_t DeviceStatus;

/*!
 * LoRaLink radio events status
 */
LoRaLinkRadioEvents_t LoRaLinkRadioEventsStatus = { .Value = 0 };

/*!
 * TxData is ready
 */
bool LoRaLinkNextTx = false;

/*
 *
 */
static void ProcessRadioRxDone( void );
static void ProcessRadioTxDone( void );
static void ProcessRadioRxTimeout( void );
static void ProcessRadioTxTimeout( void );
static void ProcessRadioRxError( void );

static void OnRadioTxDone( void );
static void OnRadioRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void OnRadioTxTimeout( void );
static void OnRadioRxError( void );
static void OnRadioRxTimeout( void );
static void OnTxDelayedTimerEvent( void *context );

static void LoRaLinkHandleIrqEvents( void );
//static void LoRaLinkDeviceHandleIrqEvents( void );
static bool scheduleTx( void );
static bool SetTxConfig( TxConfigParams_t* txConfig, TimerTime_t* txTimeOnAir );
static bool SetRxConfig( RxConfigParams_t* rxConfig );
static uint32_t GetBandwidth( uint32_t drIndex );
static void CalcBackOffTime( void );


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
	SX1276SetSyncword(LORA_SYNCWORD);

	TimerInit(&LoRaLinkCtx.TxDelayedTimer, OnTxDelayedTimerEvent);


}

LoRaLinkStatus_t LoRaLinkDeviceInit( uint8_t rxCh, uint8_t txCh, LoRaLinkSf_t sfValue, int8_t power )
{
	LoRaLinkStatus_t rc = LORALINK_STATUS_OK;


	if ( ( rxCh <= DwelltimeRange[0] && rxCh < DwelltimeRange[1] ) && ( txCh <= DwelltimeRange[0] && txCh < DwelltimeRange[1] ) )
	{
		LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_0;
		LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell0[ rxCh - DwelltimeRange[0]];
		LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell0[ txCh - DwelltimeRange[0]];
		LoRaLinkCtx.TxConfig.DutyCycle = 0;
		LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell0[SF_12 - sfValue];
	}
	else if ( ( rxCh <= DwelltimeRange[1] && rxCh < DwelltimeRange[2] ) && ( txCh <= DwelltimeRange[1] && txCh < DwelltimeRange[2] ) )
	{
		if ( sfValue >= SF_11 )
		{
			return LORALINK_STATUS_ERROR;
		}
		else
		{
			LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_1;
			LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell1[ rxCh - DwelltimeRange[1]];
			LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell1[ txCh - DwelltimeRange[1]];
			LoRaLinkCtx.TxConfig.DutyCycle = LORALINK_DUTYCYCLE;
			LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell1[SF_12 - sfValue];
		}
	}
	else
	{
		return LORALINK_STATUS_ERROR;
	}

	LoRaLinkCtx.RxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.TxPower = power;
	LoRaLinkCtx.RxConfig.RxContinuous = false;
	LoRaLinkCtx.RxConfig.WindowTimeout = LORALINK_WINDOW_TIMEOUT;

	return rc;
}



LoRaLinkStatus_t LoRaLinkModemProcess( LoRaLinkDeviceType_t devType, uint8_t devRxCh, uint8_t devTxCh, LoRaLinkSf_t sfValue, int8_t power)
{
	LoRaLinkStatus_t rc = LORALINK_STATUS_ERROR;

	if ( devType == LORALINK_DEVICE )
	{
		return rc;
	}

	if ( ( devRxCh >= DwelltimeRange[0] && devRxCh < DwelltimeRange[1] ) && ( devTxCh >= DwelltimeRange[0] && devTxCh < DwelltimeRange[1] ) )
	{
		LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_0;
		LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell0[ devTxCh - DwelltimeRange[0]];
		LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell0[ devRxCh - DwelltimeRange[0]];
		LoRaLinkCtx.TxConfig.DutyCycle = 0;
		LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell0[SF_12 - sfValue];
	}
	else if ( ( devRxCh >= DwelltimeRange[1] && devRxCh < DwelltimeRange[2] ) && ( devTxCh >= DwelltimeRange[1] && devTxCh < DwelltimeRange[2] ) )
	{
		if ( sfValue >= SF_11 )
		{
			return rc;
		}
		else
		{
			LoRaLinkCtx.LoRaLinkDwelltime = DWELLTIME_1;
			LoRaLinkCtx.RxConfig.Frequency = FreqenciesDwell1[ devTxCh - DwelltimeRange[1]];
			LoRaLinkCtx.TxConfig.Frequency = FreqenciesDwell1[ devRxCh - DwelltimeRange[1]];
			LoRaLinkCtx.TxConfig.DutyCycle = LORALINK_DUTYCYCLE;
			LoRaLinkCtx.TxConfig.PktLen = MaxPayloadDwell1[SF_12 - sfValue];
		}
	}
	else
	{
		return rc;
	}

	LoRaLinkCtx.RxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.SFValue = sfValue;
	LoRaLinkCtx.TxConfig.TxPower = power;
	LoRaLinkCtx.RxConfig.RxContinuous = true;
	LoRaLinkCtx.RxConfig.WindowTimeout = LORALINK_WINDOW_TIMEOUT;


	if ( devType == LORALINK_MODEM_TX )
	{
		DeviceStatus = DEVICE_STATE_TX_INIT;
	}
	else
	{
		DeviceStatus = DEVICE_STATE_RX_INIT;
	}

	rc = LORALINK_STATUS_OK;
	LoRaLinkApiReadParameters_t resp = { 0 };
	LoRaLinkApi_t api = { 0 };

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
			LoRaLinkApiPutRecvData( &LoRaLinkPacket );
			DeviceStatus = DEVICE_STATE_RX_INIT;
			break;

		case DEVICE_STATE_RX_TIMEOUT:
		case DEVICE_STATE_RX_ERROR:

		    // ToDo create Response Error API

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


LoRaLinkStatus_t LoRaLinkSend( uint32_t timeout )
{
	LoRaLinkNextTx = true;

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
			else
			{
				return LORALINK_STATUS_OK;
			}
			break;

		case DEVICE_STATE_CYCLE:
			TimerSetValue( &LoRaLinkCtx.TxDelayedTimer, LoRaLinkCtx.BackoffTime );
			TimerStart(&LoRaLinkCtx.TxDelayedTimer);
			DeviceStatus = DEVICE_STATE_SLEEP;
			break;

		break;

		case DEVICE_STATE_SLEEP:
			DeviceLowPowerHandler( );
			break;

		default:
			break;
		}
	}
}

LoRaLinkStatus_t LoRaLinkRecv( LoRaLinkPacket_t* pkt, uint32_t timeout )
{
	while ( true )
	{
		LoRaLinkHandleIrqEvents();

		switch ( (int)DeviceStatus )
		{
		case DEVICE_STATE_RX_INIT:
			SetRxConfig( &LoRaLinkCtx.RxConfig );
			SX1276SetRx(timeout);

			if ( timeout > 0 )
			{
				DeviceStatus = DEVICE_STATE_SLEEP;
			}
			break;

		case DEVICE_STATE_SLEEP:
			DeviceLowPowerHandler( );
			break;

		case DEVICE_STATE_RX_DONE:
			pkt = &LoRaLinkPacket;
			return LORALINK_STATUS_OK;

		case DEVICE_STATE_RX_TIMEOUT:
			pkt = NULL;
			return LORALINK_STATUS_RX_TIMEOUT;

		default:
			pkt = NULL;
			return LORALINK_STATUS_ERROR;
		}
	}
}

void LoRaLinkSetTxData( LoRaLinkPacket_t* LoRaLinkPkt )
{
	LoRaLinkCtx.TxMsg.Buffer = LoRaLinkCtx.PktBuffer;

	// encrypt Payload
	LoRaLinkCryptoSecureMessage(LoRaLinkPkt);

	LoRaLinkCtx.PktBufferLen = LoRaLinkPkt->BufSize;
}

LoRaLinkStatus_t LoRaLinkSerializeData(LoRaLinkPacket_t* pkt)
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
	pkt->FRMPayloadSize = rxData->Size - LORALINK_HDR_LEN;
	memcpy1(pkt->FRMPayload, pos, pkt->FRMPayloadSize );
	setUint32(pos + pkt->FRMPayloadSize, pkt->MIC);

	return LORALINK_STATUS_OK;
}

static bool scheduleTx( void )
{

	CalcBackOffTime();
	if ( LoRaLinkCtx.BackoffTime > 0 )
	{
		DeviceStatus = DEVICE_STATE_CYCLE;
		return true;
	}

	if ( SX1276IsChannelFree( MODEM_FSK, LoRaLinkCtx.TxConfig.Frequency, LORALINK_RSSI_THRESH, LORALINK_MAX_CARRIERSENSE_TIME ) == true )
	{
		SetTxConfig( &LoRaLinkCtx.TxConfig, &LoRaLinkCtx.TxTimeOnAir );

		SX1276Send( LoRaLinkCtx.PktBuffer,  LoRaLinkCtx.PktBufferLen );
		return false;
	}
	return true;
}


static bool SetRxConfig( RxConfigParams_t* rxConfig )
{
    RadioModems_t modem = MODEM_LORA;
    uint8_t maxPayload = 0;
    int8_t dr = SF_12 - rxConfig->SFValue;
    uint32_t bandwidth = GetBandwidth(dr);

    if( SX1276GetStatus( ) != RF_IDLE )
    {
        return false;
    }

    SX1276SetChannel( rxConfig->Frequency );

	SX1276SetRxConfig( modem, bandwidth, (uint32_t)rxConfig->SFValue, LORALINK_CODERATE, 0, LORALINK_PREAMBLE_LENGTH, rxConfig->WindowTimeout, false, 0, true, false, 0, false, rxConfig->RxContinuous );

    if ( LoRaLinkCtx.LoRaLinkDwelltime == DWELLTIME_0 )
    {
    	maxPayload = MaxPayloadDwell0[dr];
    }
    else
    {
    	maxPayload = MaxPayloadDwell1[dr];
    }

    SX1276SetMaxPayloadLength( modem, maxPayload + LORALINK_HDR_LEN + LORALINK_MIC_LEN);

    return true;
}


static bool SetTxConfig( TxConfigParams_t* txConfig, TimerTime_t* txTimeOnAir )
{
    RadioModems_t modem = MODEM_LORA;
    int8_t phyDr = txConfig->SFValue;
    uint32_t bandwidth = GetBandwidth(SF_12 - phyDr);

    // Setup the radio frequency
    SX1276SetChannel( txConfig->Frequency );

	SX1276SetTxConfig( modem, txConfig->TxPower, 0, bandwidth, phyDr, LORALINK_CODERATE , LORALINK_PREAMBLE_LENGTH, false, true, 0, 0, false, 4000 );

    // Setup maximum payload length of the radio driver
    SX1276SetMaxPayloadLength( modem, LoRaLinkCtx.PktBufferLen );
    // Get the time-on-air of the next tx frame
    *txTimeOnAir = SX1276GetTimeOnAir( modem, LoRaLinkCtx.PktBufferLen );

    return true;
}




static uint32_t GetBandwidth( uint32_t drIndex )
{
    switch( Bandwidths[drIndex] )
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

		if ( elapsed >= backoff )
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
    }
}

/*
static void LoRaLinkDeviceHandleIrqEvents( void )
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
            ProcessDeviceRadioTxDone( );
        }
        if( events.Events.RxDone == 1 )
        {
            ProcessRadioRxDone( );
        }
        if( events.Events.TxTimeout == 1 )
        {
            ProcessDeviceRadioTxTimeout( );
        }
        if( events.Events.RxError == 1 )
        {
            ProcessDeviceRadioRxError( );
        }
        if( events.Events.RxTimeout == 1 )
        {
            ProcessDeviceRadioRxTimeout( );
        }
    }
}*/

/*
 * Interrupt callbacks for Modem
 */
static void ProcessRadioTxDone( void )
{
	LoRaLinkCtx.LastTxDoneTime = TxDoneParams.CurTime;

	// Create Response Ok API


	DeviceStatus = DEVICE_STATE_TX_DONE;

}

static void ProcessRadioRxDone( void )
{
	SX1276SetSleep();
	LoRaLinkApiGetRxData( &LoRaLinkPacket, &RxDoneParams );
//	if ( LoRaLinkPacket.PanId == LoRaLinkCtx.LoRaLinkPanId && LoRaLinkPacket.DestAddr == LoRaLinkCtx.LoRaLinkDeviceAddr)
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
	DeviceStatus = DEVICE_STATE_TX_DONE;
}

static void ProcessRadioRxError( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_RX_ERROR;
}

/*
 * Interrupt callbacks for Device
 */
/*
static void ProcessDeviceRadioRxDone( void )
{
	SX1276SetSleep();
	LoRaLinkApiGetRxData( &LoRaLinkCtx.TxMsg, &RxDoneParams );
	DeviceStatus = DEVICE_STATE_RX_DONE;
}


static void ProcessDeviceRadioTxDone( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_TX_DONE;
}

static void ProcessDeviceRadioRxTimeout( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_RX_TIMEOUT;
}

static void ProcessDeviceRadioTxTimeout( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_TX_TIMEOUT;
}

static void ProcessDeviceRadioRxError( void )
{
	SX1276SetSleep();
	DeviceStatus = DEVICE_STATE_RX_ERROR;
}
*/

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
	LoRaLinkCtx.BackoffTime = 0;
	DeviceStatus = DEVICE_STATE_TX;
}
