/***************************************************************************************
 *
 * MQTTSNClient.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#ifndef MQTTSNCLIENT_H_
#define MQTTSNCLIENT_H_

#include "MQTTSNDefines.h"
#include "LoRaLinkTypes.h"

typedef enum
{
	CS_GW_LOST,
	CS_SEARCHING,
	CS_CONNECTING,
	CS_WAIT_WILLTOPICREQ,
	CS_SEND_WILLTOPIC,
	CS_WAIT_WILLMSGREQ,
	CS_SEND_WILLMSG,
	CS_WAIT_CONNACK,
	CS_ACTIVE,
	CS_WAIT_PINGRESP,
	CS_DISCONNECTING,
	CS_SLEEPING,
	CS_ASLEEP,
	CS_AWAKE,
	CS_DISCONNECTED,
}MQTTSNClientState_t;

typedef enum
{
	MQTTSN_STATE_OK,
	MQTTSN_STATE_RETRY_OUT,
	MQTTSN_STATE_INVALID_MSGID,
	MQTTSN_STATE_INVALID_STATUS,
}MQTTSNState_t;

void MQTTSNClientInit( MQTTSNConf_t* conf );
void MQTTSNQoSM1Init( uint8_t*  prefixOfClientId, uint8_t gwAddr );
void Connect( void );
void Reconnect( void );
uint16_t GetNextMsgId( void );
LoRaLinkStatus_t WriteMsg( uint8_t* msg );
uint8_t GetMessage( uint32_t timeout );
void Disconnect( uint32_t ms );
void RestartPingRequestTimer( void );
void CheckPingRequest( void );
uint8_t* GetMsgType( uint8_t msgType );

#endif /* MQTTSNCLIENT_H_ */


