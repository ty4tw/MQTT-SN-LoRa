/**************************************************************************************
 *
 * MQTTSNRegister.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include "MQTTSNRegister.h"
#include "MQTTSNDefines.h"
#include "MQTTSNClient.h"
#include "MQTTSNTopic.h"
#include "MQTTSNPublish.h"
#include "utilities.h"
#include "systime.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
	uint8_t topicName[ MQTTSN_MAX_MSG_LENGTH + 1 ];
	uint16_t msgId;
    uint8_t      retryCount;
}MQTTSNRegister_t;


MQTTSNRegister_t RegisterMsg = { 0 };


static MQTTSNState_t SendRegisterMsg( MQTTSNRegister_t* msg );


static void resetRegisterMsg( MQTTSNRegister_t* msg )
{
	memset1( (uint8_t*)msg, 0, sizeof( MQTTSNRegister_t ) );
}

bool IsRegisterDone( void )
{
	return *RegisterMsg.topicName == 0;
}

MQTTSNState_t RegisterTopic( uint8_t* topicName )
{
	memcpy1( RegisterMsg.topicName, topicName, strlen( (const char*)topicName) );
	RegisterMsg.msgId = GetNextMsgId();
	return SendRegisterMsg( &RegisterMsg );
}

MQTTSNState_t ResponceRegAck( uint16_t msgId, uint16_t topicId )
{
	if ( RegisterMsg.msgId == msgId )
	{
		uint8_t topicType = strlen((const char*) RegisterMsg.topicName) > 2 ? MQTTSN_TOPIC_TYPE_NORMAL : MQTTSN_TOPIC_TYPE_SHORT;
		SetTopicId( RegisterMsg.topicName, topicId, topicType );

		SendPublishSuspend( RegisterMsg.topicName, topicId, topicType );
		resetRegisterMsg( &RegisterMsg );
		return MQTTSN_STATE_OK;
	}
	return MQTTSN_STATE_INVALID_MSGID;
}

void ResponceRegister( uint8_t* msg, uint16_t msglen )
{
	// *msg is terminated with 0x00 by Network::getMessage()
	uint8_t regack[7];

	regack[0] = 7;
	regack[1] = MQTTSN_TYPE_REGACK;
	memcpy1(regack + 2, msg + 2, 4);

	MQTTSNTopic_t* tp = GetTopicMatch( msg + 5 );

	if ( tp != NULL )
	{
		TopicCallback callback = tp->Callback;
		void* topicName = calloc( strlen((char*) msg + 5) + 1, sizeof(char) );
		MQTTSNTopicAdd( topicName, 0, MQTTSN_TOPIC_TYPE_NORMAL, callback );
		regack[6] = MQTTSN_RC_ACCEPTED;
	}
	else
	{
		regack[6] = MQTTSN_RC_REJECTED_INVALID_TOPIC_ID;
	}
	WriteMsg(regack);
}

static LoRaLinkStatus_t SendRegister( MQTTSNRegister_t* msg )
{
	uint8_t buf[ MQTTSN_MAX_MSG_LENGTH + 1 ];
	LoRaLinkStatus_t stat = LORALINK_STATUS_ERROR;

	buf[0] = 6 + strlen( (const char*)msg->topicName );
	buf[1] = MQTTSN_TYPE_REGISTER;
	buf[2] = buf[3] = 0;
	setUint16( buf + 4, msg->msgId );
	strcpy( (char*) buf + 6, (const char*)msg->topicName );
	Connect();
	stat = WriteMsg(buf);
	msg->retryCount++;
	return stat;
}

static MQTTSNState_t SendRegisterMsg( MQTTSNRegister_t* msg )
{
	while ( msg->retryCount < MQTTSN_RETRY_COUNT )
	{
		LoRaLinkStatus_t stat = SendRegister( msg );

		if ( stat == LORALINK_STATUS_OK )
		{
			if ( GetMessage( MQTTSN_TIMEOUT_MS ) > 0  )
			{
				return MQTTSN_STATE_OK;
			}
		}
	}
	return MQTTSN_STATE_RETRY_OUT;
}

