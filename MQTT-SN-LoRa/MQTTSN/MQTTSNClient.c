/**************************************************************************************
 *
 * MQTTSNClient.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include "MQTTSNDefines.h"
#include "MQTTSNClient.h"
#include "MQTTSNTopic.h"
#include "MQTTSNPublish.h"
#include "MQTTSNRegister.h"
#include "MQTTSNSubscribe.h"
#include "TaskMgmt.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "LoRaLink.h"
#include "utilities.h"
#include "sx1276.h"

extern void OnConnect( void );

static const char* packet_names[] =
{
	"ADVERTISE", "SEARCHGW", "GWINFO", "RESERVED", "CONNECT", "CONNACK",
	"WILLTOPICREQ", "WILLTOPIC", "WILLMSGREQ", "WILLMSG", "REGISTER", "REGACK",
	"PUBLISH", "PUBACK", "PUBCOMP", "PUBREC", "PUBREL", "RESERVED",
	"SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP",
	"DISCONNECT", "RESERVED", "WILLTOPICUPD", "WILLTOPICRESP", "WILLMSGUPD",
	"WILLMSGRESP"
};


static uint16_t     NextMsgId = 1;
static uint8_t*     ClientId = NULL;
static uint8_t*     WillTopic = NULL;
static uint8_t*     WillMsg = NULL;
static uint8_t      CleanSession = 0;
static uint8_t      RetainWill = 0;
static uint8_t      QosWill = 0;
static uint8_t      GwId = 0;
static TimerTime_t  TkeepAliveMs = 0;
static TimerTime_t  Tadv = 0;
static TimerTime_t  TsleepMs = 0;
static SysTime_t    TimeSend = { 0 };
static uint8_t      RetryCount = 0;;
static uint8_t      ConnectRetry = 0;
static uint8_t      PingRetryCount = 0;

static TimerEvent_t KeepAliveTimer = { 0 };
static TimerEvent_t SleepTimer = { 0 };

static bool  PingRequestFlg = false;
static bool  SleepTimeupFlg = false;
static bool  AsleepFlg      = false;
static bool  WaitPublishFlg = false;
static bool  onConnectExecFlg = false;

static bool  QoSM1DeviceFlg = false;


uint8_t      Msg[MQTTSN_MAX_MSG_LENGTH + 1];
uint8_t*     MQTTSNMsg;
uint16_t     GwPanId = 0;
uint8_t      GwDevAddr = 0;
uint8_t      SenderDevAddr = 0;
uint16_t     SenderPanId = 0;
uint16_t     RssiValue = 0;
uint16_t     SnrValue = 0;

uint8_t      GwAliveTimeupFlg = 0;

MQTTSNClientState_t ClientStatus = CS_GW_LOST;

LoRaLinkPacket_t RecvPacket = { 0 };

/*
 * Predefined functions
 */
static void GetConnectResponce( uint32_t timeout );
static uint8_t GetDisconnectResponce( uint32_t timeout );
static uint8_t ReadMsg( uint32_t timeout );
static void OnKeepAliveTimeupEvent( void *context );
static void OnSleepTimeupEvent( void *context );
static void StartClientWakeupTimer( uint32_t ms );
static void StopPingRequestTimer( void );
//static void StopClientWakeupTimer( void );


void MQTTSNClientInit( MQTTSNConf_t* conf )
{
	uint16_t panId = LoRaLinkGetPanId();
	uint8_t devAddr = LoRaLinkGetSourceAddr();

	ClientStatus = CS_GW_LOST;
	WillTopic = (uint8_t*)conf->willTopic;
	WillMsg = (uint8_t*)conf->willMsg;
	QosWill = conf->willQos;
	RetainWill = conf->willRetain;
	CleanSession = conf->cleanSession;
	TkeepAliveMs = conf->keepAlive * (uint32_t)1000;

	if ( ClientId != NULL )
	{
		free( ClientId );
	}
	ClientId = malloc( strlen( conf->clientId) + 6 );
	sprintf( (char*)ClientId, "%s%04x%02x", conf->clientId, panId, devAddr );

	TimerInit( &KeepAliveTimer, OnKeepAliveTimeupEvent );
	TimerInit( &SleepTimer, OnSleepTimeupEvent );
}

void MQTTSNQoSM1Init( uint8_t*  prefixOfClientId )
{
	uint16_t panId = LoRaLinkGetPanId();
	uint8_t devAddr = LoRaLinkGetSourceAddr();

	ClientStatus = CS_ACTIVE;

	uint8_t len = strlen( (const char*)prefixOfClientId );

	ClientId = (uint8_t*)malloc( len + 6 );
	sprintf( (char*)ClientId, "%s%04x%02x", prefixOfClientId, panId, devAddr );

	TimerInit( &KeepAliveTimer, OnKeepAliveTimeupEvent );
	TimerInit( &SleepTimer, OnSleepTimeupEvent );
}

uint8_t* GetMsgType( uint8_t msgType )
{
	return (uint8_t*)packet_names[ msgType ];
}

void Connect( void )
{
	uint8_t* pos;

	if ( QoSM1DeviceFlg == true )
	{
		return;
	}

	while ( ClientStatus != CS_ACTIVE )
	{
		pos = Msg;

		if ( ClientStatus == CS_CONNECTING || ClientStatus == CS_DISCONNECTED )
		{
			uint8_t clientIdLen = strlen( (const char*)ClientId );
			*pos++ = 6 + clientIdLen;
			*pos++ = MQTTSN_TYPE_CONNECT;
			pos++;
			if ( CleanSession )
			{
				Msg[2] = MQTTSN_FLAG_CLEAN;
			}
			*pos++ = MQTTSN_PROTOCOL_ID;
			setUint16( pos, TkeepAliveMs );
			pos += 2;
			strncpy( (char*)pos, (const char*)ClientId, clientIdLen);
			Msg[6 + clientIdLen] = 0;

			if ( ( strlen( (const char*)WillMsg ) > 0 ) && ( strlen( (const char*)WillTopic ) > 0 ) )
			{
				Msg[2] = Msg[2] | MQTTSN_FLAG_WILL;   // CONNECT
				ClientStatus = CS_WAIT_WILLTOPICREQ;
			}
			else
			{
				ClientStatus = CS_WAIT_CONNACK;
			}

			RetryCount = MQTTSN_RETRY_COUNT;

			DLOG("Send %s\r\n", packet_names[ Msg[1] ] );
			WriteMsg( Msg );
		}
		else if ( ClientStatus == CS_SEND_WILLTOPIC )
		{
			*pos++ = 3 + (uint8_t) strlen( (const char*)WillTopic );
			*pos++ = MQTTSN_TYPE_WILLTOPIC;
			*pos++ = QosWill | RetainWill;
			strcpy((char*)pos, (const char*)WillTopic);
			ClientStatus = CS_WAIT_WILLMSGREQ;
			RetryCount = MQTTSN_RETRY_COUNT;

			DLOG("Send %s\r\n", packet_names[ Msg[1] ] );
			WriteMsg( Msg );
		}
		else if ( ClientStatus == CS_SEND_WILLMSG )
		{
			*pos++ = 2 +(uint8_t)strlen( (const char*)WillMsg );
			*pos++ = MQTTSN_TYPE_WILLMSG;
			strcpy( (char*)pos, (const char*)WillMsg);
			ClientStatus = CS_WAIT_CONNACK;
			RetryCount = MQTTSN_RETRY_COUNT;

			DLOG("Send %s\r\n", packet_names[ Msg[1] ] );
			WriteMsg( Msg );
		}
		else  if ( ClientStatus == CS_GW_LOST )
		{
			*pos++ = 3;
			*pos++ = MQTTSN_TYPE_SEARCHGW;
			*pos = 0;                        // SERCHGW
			ClientStatus = CS_SEARCHING;
			RetryCount = MQTTSN_RETRY_COUNT;

			DLOG("Send %s\r\n", packet_names[ Msg[1] ] );
			WriteMsg( Msg );
		}

		GetConnectResponce( MQTTSN_TIMEOUT_MS );
	}
	return;
}

void Reconnect( void )
{
	ClientStatus = CS_DISCONNECTED;
	Connect();
}

void Disconnect( uint32_t ms )
{
	if ( ClientStatus == CS_GW_LOST || ClientStatus == CS_DISCONNECTED || ClientStatus == CS_DISCONNECTING )
	{
		return;
	}

	ClientStatus = CS_DISCONNECTING;
	TsleepMs = ms;

	Msg[1] = MQTTSN_TYPE_DISCONNECT;

	if ( ms > 0 )
	{
		Msg[0] = 4;
		setUint16( (uint8_t*)Msg + 2, ms );
	}
	else
	{
		Msg[0] = 2;
		StartClientWakeupTimer( ms );
	}

	// Try to recv PUBLISH before send DISCONNECT
	uint8_t len = 1;

	while ( len != 0 && WaitPublishFlg == true )
	{
		len = GetMessage( MQTTSN_TIMEOUT_MS );
	}


	RetryCount = 0;

	DLOG("Send %s %ldms\r\n", packet_names[ Msg[1] ], ms );
	WriteMsg( Msg );

	while ( ClientStatus != CS_DISCONNECTED && ClientStatus != CS_ASLEEP && RetryCount-- >= 0 )
	{
		if ( GetDisconnectResponce( MQTTSN_TIMEOUT_MS ) > 0 )
		{
			return;
		}
	}
}

static void GetConnectResponce( uint32_t timeout )
{
	uint8_t len = ReadMsg( timeout );

	if ( len == 0 )
	{
		if ( Msg[1] == MQTTSN_TYPE_CONNECT )
		{
			ConnectRetry++;
		}

		if ( RetryCount++ < MQTTSN_RETRY_COUNT )
		{
			WriteMsg( Msg );
		}
		else
		{
			if (ClientStatus >= CS_CONNECTING && ConnectRetry < MQTTSN_RETRY_COUNT )
			{
				ClientStatus = CS_CONNECTING;
			}
			else
			{
				ClientStatus = CS_GW_LOST;
				GwId = 0;
			}
		}
	}
	else
	{
		DLOG("Recv %s\r\n", packet_names[ MQTTSNMsg[1] ] );

		if ( MQTTSNMsg[1] == MQTTSN_TYPE_GWINFO && ClientStatus == CS_SEARCHING)
		{
			GwId = MQTTSNMsg[2];
			GwDevAddr = SenderDevAddr;
			ClientStatus = CS_CONNECTING;
		}
		else if (MQTTSNMsg[1] == MQTTSN_TYPE_WILLTOPICREQ && ClientStatus == CS_WAIT_WILLTOPICREQ)
		{
			ClientStatus = CS_SEND_WILLTOPIC;
		}
		else if (MQTTSNMsg[1] == MQTTSN_TYPE_WILLMSGREQ && ClientStatus == CS_WAIT_WILLMSGREQ)
		{
			ClientStatus = CS_SEND_WILLMSG;
		}
		else if (MQTTSNMsg[1] == MQTTSN_TYPE_CONNACK && ClientStatus == CS_WAIT_CONNACK)
		{
			if (MQTTSNMsg[2] == MQTTSN_RC_ACCEPTED)
			{
				RestartPingRequestTimer();
				ConnectRetry = 0;
				GwPanId = RecvPacket.PanId;
				ClientStatus = CS_ACTIVE;

				if ( CleanSession == true )
				{
					ClearTopicTable();
				}

				if ( AsleepFlg == false &&  ( CleanSession == false && onConnectExecFlg == false ) )
				{
					OnConnect();  // SUBSCRIBEs are conducted
					onConnectExecFlg = true;
				}

				GetMessage( MQTTSN_TIMEOUT_MS );  // try to receive PUBLIC. GW sends PUBLISH if it has retain message.
			}
			else
			{
				ClientStatus = CS_CONNECTING;
			}
		}
	}
}



static uint8_t GetDisconnectResponce( uint32_t timeout )
{
	int len = ReadMsg( timeout );

	if (len == 0)
	{
		if ( RetryCount++ >= MQTTSN_RETRY_COUNT )
		{
			WriteMsg( Msg );
		}
		else
		{
			ClientStatus = CS_GW_LOST;
			GwId = 0;
			return 1;
		}
	}
	else
	{

		if ( MQTTSNMsg[1] == MQTTSN_TYPE_DISCONNECT)
		{
			DLOG("Recv %s\r\n", packet_names[ MQTTSNMsg[1] ] );

			if ( TsleepMs > 0 )
			{
				ClientStatus = CS_ASLEEP;
				AsleepFlg = true;
				StartClientWakeupTimer( TsleepMs );
			}
			else
			{
				AsleepFlg = false;
				ClientStatus = CS_DISCONNECTED;
				StopPingRequestTimer();
			}
		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_PUBLISH)
		{
			DLOG("Recv %s\r\n",packet_names[ MQTTSNMsg[1] ] );
			DLOG("       msgId:%04x topicType:%02x topicId:%02x%02x\r\n", getUint16( (const uint8_t*)(MQTTSNMsg+ 5) ), MQTTSNMsg[1] & 0x03, MQTTSNMsg[3], MQTTSNMsg[4] );

			Published( MQTTSNMsg, MQTTSNMsg[0] );
		}
	}
	return len;
}

uint8_t GetMessage( uint32_t timeout )
{
	uint8_t len = ReadMsg( timeout );

	if ( len > 0 )
	{
		DLOG("Recv %s\r\n",packet_names[ MQTTSNMsg[1] ] );

		if ( MQTTSNMsg[1] == MQTTSN_TYPE_PUBLISH)
		{
			DLOG("       msgId:%04x topicType:%02x topicId:%02x%02x\r\n", getUint16( (const uint8_t*)(MQTTSNMsg+ 5) ), MQTTSNMsg[1] & 0x03, MQTTSNMsg[3], MQTTSNMsg[4] );

			Published( MQTTSNMsg, MQTTSNMsg[0] );
		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_PUBACK )
		{
			DLOG("       msgId:%04x topicType:%02x topicId:%02x%02x\r\n", MQTTSNMsg[1], getUint16( (const uint8_t*)(MQTTSNMsg+ 5) ), MQTTSNMsg[2], MQTTSNMsg[3] );

			ResponcePublish( MQTTSNMsg, len );

		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_PUBCOMP || MQTTSNMsg[1] == MQTTSN_TYPE_PUBREC
					|| MQTTSNMsg[1] == MQTTSN_TYPE_PUBREL)
		{
			DLOG("       msgId:%04x\r\n", getUint16( (const uint8_t*)(MQTTSNMsg+ 2) ) );
			ResponcePublish( MQTTSNMsg, (uint16_t) len );
		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_SUBACK || MQTTSNMsg[1] == MQTTSN_TYPE_UNSUBACK)
		{
			DLOG("       msgId:%04x topicId:%02x%02x\r\n", getUint16( (const uint8_t*)(MQTTSNMsg+ 5) ), MQTTSNMsg[3], MQTTSNMsg[4] );

			if ( MQTTSNMsg[1] == MQTTSN_TYPE_SUBACK )
			{
				WaitPublishFlg = true;
			}

			ResponceSubscribe( MQTTSNMsg );
		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_REGISTER)
		{
			DLOG("       msgId:%04x topicId:%02x%02x\r\n", getUint16( (const uint8_t*)(MQTTSNMsg+ 5) ), MQTTSNMsg[3], MQTTSNMsg[4] );

			ResponceRegister( MQTTSNMsg, len );

		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_REGACK)
		{
			DLOG("       msgId:%04x topicId:%02x%02x\r\n", getUint16( (const uint8_t*)(MQTTSNMsg+ 5) ), MQTTSNMsg[2], MQTTSNMsg[3] );

			ResponceRegAck(getUint16( MQTTSNMsg + 4), getUint16( MQTTSNMsg + 2));

		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_PINGRESP)
		{
			ResponceRegAck(getUint16( MQTTSNMsg + 4), getUint16( MQTTSNMsg + 2));
			RestartPingRequestTimer();

			if ( ClientStatus == CS_AWAKE )
			{
				ClientStatus = CS_ASLEEP;
			}
			else
			{
				ClientStatus = CS_ACTIVE;
			}
		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_DISCONNECT)
		{
			ClientStatus = CS_DISCONNECTED;
			StopPingRequestTimer( );
		}
		else if ( MQTTSNMsg[1] == MQTTSN_TYPE_ADVERTISE)
		{
			uint16_t duration = getUint16( (const uint8_t*)(MQTTSNMsg + 3) );
			if ( duration < 61 )
			{
				Tadv = duration * 1500;
			}
			else
			{
				Tadv = duration * 1100;
			}
		}
	}
	else
	{
		DLOG("Recv Timeout\r\n");
	}
	return len;
}

static MQTTSNState_t SendPingReq( uint8_t* msg )
{
	while ( (ClientStatus !=  CS_ACTIVE) && (ClientStatus != CS_ASLEEP)  )
	{
		LoRaLinkStatus_t stat = WriteMsg( msg );

		if ( stat == LORALINK_STATUS_OK )
		{

			if ( GetMessage( MQTTSN_TIMEOUT_MS ) > 0 )
			{
				free( msg );
				return MQTTSN_STATE_OK;
			}
		}
	}

	free( msg );
	ClientStatus = CS_GW_LOST;
	GwId = 0;
	DLOG("     !!! PINGRESP Recv Timeout\n");
	return MQTTSN_STATE_RETRY_OUT;
}

static MQTTSNState_t SendPingReqMsg( void )
{
	PingRetryCount = 0;

	uint8_t* msg = NULL;
	uint8_t len = strlen( (const char*)ClientId );

	if ( ClientStatus == CS_ASLEEP )
	{
		msg = (uint8_t*)malloc( len + 2 + 1 );
		msg[0] = len + 2;
		msg[1] = MQTTSN_TYPE_PINGREQ;
		memcpy1( msg + 2, ClientId, len );
		msg[ 2 + len + 1] = 0;
		ClientStatus = CS_AWAKE;

		DLOG("Send %s ClientId: %s\r\n", "PINGREQ" , msg + 3 + len );
		return SendPingReq( msg );
	}
	else if ( ClientStatus == CS_ACTIVE )
	{
		msg = (uint8_t*)malloc( 2 );
		msg[0] = 2;
		msg[1] = MQTTSN_TYPE_PINGREQ;
		ClientStatus = CS_WAIT_PINGRESP;

		DLOG("Send %s w/o ClientId\r\n", "PINGREQ" );
		return SendPingReq( msg );
	}
	return MQTTSN_STATE_INVALID_STATUS;
}

void CheckPingRequest( void )
{
	if ( PingRequestFlg == true )
	{
		PingRequestFlg = false;
		SendPingReqMsg( );
	}
}

static void OnKeepAliveTimeupEvent( void *context )
{
	PingRequestFlg = true;
}

static void OnSleepTimeupEvent( void *context )
{
	SleepTimeupFlg = true;
}


void RestartPingRequestTimer( void )
{
	TimerStop( &KeepAliveTimer );
	TimerSetValue( &KeepAliveTimer, TkeepAliveMs );
	TimerStart( &KeepAliveTimer );
	PingRequestFlg = false;
}

static void StopPingRequestTimer( void )
{
	TimerStop( &KeepAliveTimer );
	PingRequestFlg = false;
}

static void StartClientWakeupTimer( uint32_t ms )
{
	TimerSetValue( &SleepTimer, ms );
	TimerStart( &SleepTimer );
	SleepTimeupFlg = false;
}

//void static StopClientWakeupTimer( void )
//{
//	TimerStop( &SleepTimer );
//	SleepTimeupFlg = 0;
//}


LoRaLinkStatus_t WriteMsg( uint8_t* msg )
{
	uint16_t len = 0;
	LoRaLinkStatus_t stat = LORALINK_STATUS_ERROR;
	uint8_t devAddr = 0;

	len = msg[0];

	if (msg[0] == 3 && msg[1] == MQTTSN_TYPE_SEARCHGW )
	{
		devAddr = LORALINK_MULTICAST_ADDR;
	}
	else
	{
		devAddr = GwDevAddr;
	}

	stat = LoRaLinkSend( devAddr, MQTT_SN, msg, len, 4000 );

	if ( stat == LORALINK_STATUS_OK )
	{
		TimeSend = SysTimeGet();
	}
	return stat;
}


static uint8_t ReadMsg( uint32_t timeout )
{
	uint8_t len = 0;
	LoRaLinkClearPacket( & RecvPacket );
	MQTTSNMsg = NULL;

	if ( LoRaLinkRecvPacket( &RecvPacket, timeout ) == LORALINK_STATUS_OK )
	{
		RssiValue = RecvPacket.Rssi;
		SnrValue = RecvPacket.Snr;
		SenderPanId = RecvPacket.PanId;
		SenderDevAddr = RecvPacket.SourceAddr;

		if ( RecvPacket.FRMPayloadType == MQTT_SN )
		{
			len = RecvPacket.FRMPayloadSize;
			MQTTSNMsg = RecvPacket.FRMPayload;
		}
	}
	return len;
}

uint16_t GetNextMsgId( void )
{
	return NextMsgId++;
}
