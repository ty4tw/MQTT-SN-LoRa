/**************************************************************************************
 *
 * MQTTSNSubscribe.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include "MQTTSNSubscribe.h"
#include "MQTTSNDefines.h"
#include "MQTTSNClient.h"
#include "MQTTSNTopic.h"
#include "TaskMgmt.h"
#include "utilities.h"
#include "systime.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define SUB_DONE   1
#define SUB_READY  0



typedef struct subElement
{
    TopicCallback callback;
    uint8_t   msgType;
    uint16_t  msgId;
    uint8_t   topicName[ MQTTSN_MAX_TOPIC_LEN + 1 ];
    uint16_t  topicId;
    uint8_t   topicType;
    MQTTSNQos_t   qos;
    uint8_t   retryCount;
} MQTTSNSubscribe_t;

extern OnPublishList_t theOnPublishList[];

static MQTTSNState_t SendSubscribeMsg( MQTTSNSubscribe_t* msg );

/*
 *  Subscribe Message
 */
MQTTSNSubscribe_t  SubscribeMsg = { 0 };

void ClearSubscribeMsg( MQTTSNSubscribe_t* msg )
{
	memset1( (uint8_t*)msg, 0, sizeof( MQTTSNSubscribe_t ) );
}

void OnConnect( void )
{
	for ( uint8_t i = 0; theOnPublishList[i].topicName != 0; i++ )
	{
		SubscribeByName( theOnPublishList[i].topicName, theOnPublishList[i].pubCallback, theOnPublishList[i].qos );
	}
}

void SubscribeByName( uint8_t* topicName, TopicCallback onPublish, MQTTSNQos_t qos )
{
	uint8_t len = strlen( (const char*)topicName );

	if ( len <= 2 )
	{
		SubscribeMsg.topicType = MQTTSN_TOPIC_TYPE_SHORT;
	}
	else
	{
		SubscribeMsg.topicType = MQTTSN_TOPIC_TYPE_NORMAL;
	}

	SubscribeMsg.msgType = MQTTSN_TYPE_SUBSCRIBE;
	memcpy1( SubscribeMsg.topicName, topicName, len );
	SubscribeMsg.callback = onPublish;
	SubscribeMsg.qos = qos;
	SendSubscribeMsg( &SubscribeMsg );
}


void SubscribeById( uint16_t topicId, uint8_t topicType, TopicCallback onPublish, MQTTSNQos_t qos )
{
	SubscribeMsg.msgType = MQTTSN_TYPE_SUBSCRIBE;
	SubscribeMsg.topicId = topicId;
	SubscribeMsg.topicType = topicType;
	SubscribeMsg.callback = onPublish;
	SubscribeMsg.qos = qos;
	SendSubscribeMsg( &SubscribeMsg );
}

void UnsubscribeByName( uint8_t* topicName)
{
	memcpy1( SubscribeMsg.topicName, topicName, strlen( (const char*)topicName ) );
	SubscribeMsg.msgType = MQTTSN_TYPE_UNSUBSCRIBE;
	SubscribeMsg.topicType = MQTTSN_TOPIC_TYPE_NORMAL;

	SendSubscribeMsg( &SubscribeMsg );
}

void UnsubscribeById(uint16_t topicId, uint8_t topicType)
{
	SubscribeMsg.topicId = topicId;
	SubscribeMsg.topicType = topicType;

	SendSubscribeMsg( &SubscribeMsg );
}

void ResponceSubscribe( uint8_t* resp )
{
	if ( resp[1] == MQTTSN_TYPE_SUBACK )
	{
		uint16_t topicId;
		uint8_t topicType = resp[2] & 0x03;
		uint16_t msgId = getUint16( resp + 5 );
		uint8_t rc = resp[7];

		if ( topicType == MQTTSN_TOPIC_TYPE_SHORT )
		{
			topicId = *(resp + 3) << 8;
			topicId |= *( resp + 4 );
		}
		else
		{
			topicId = getUint16( resp + 3 );
		}

		if ( SubscribeMsg.msgId == msgId )
		{
			if ( rc == MQTTSN_RC_ACCEPTED )
			{
				SetTopicId( SubscribeMsg.topicName, topicId, topicType );
			}
			ClearSubscribeMsg( &SubscribeMsg );
		}
	}
	else if ( resp[1] == MQTTSN_TYPE_UNSUBACK )
	{
		uint16_t msgId = getUint16( resp + 2 );

		if ( SubscribeMsg.msgId == msgId )
		{
			// NOP;
		}
		ClearSubscribeMsg( &SubscribeMsg );
	}
}


static MQTTSNState_t SendSubscribeMsg( MQTTSNSubscribe_t* msg )
{
	uint8_t buf[MQTTSN_MAX_MSG_LENGTH + 1];
	uint8_t len = strlen( (const char*)msg->topicName);

	if ( (msg->topicType == MQTTSN_TOPIC_TYPE_NORMAL) || (msg->topicType == MQTTSN_TOPIC_TYPE_SHORT) )
	{
		buf[0] = 5 + len;
		strcpy( (char*) buf + 5, (const char*)msg->topicName );
	}
	else if ( msg->topicType == MQTTSN_TOPIC_TYPE_PREDEFINED )
	{
		buf[0] = 7;
 		setUint16( buf + 5, msg->topicId );
	}
	else
	{
		return LORALINK_STATUS_PARAMETER_INVALID;
	}

	buf[1] = msg->msgType;
	buf[2] = msg->qos | msg->topicType;

	if ( msg->retryCount == 0 )
	{
		msg->msgId = GetNextMsgId();
	}

	if ( (msg->retryCount > 0 ) && msg->msgType == MQTTSN_TYPE_SUBSCRIBE )
	{
		buf[2] |= MQTTSN_FLAG_DUP;
	}

	setUint16( buf + 3, msg->msgId );

	MQTTSNTopicAdd( msg->topicName, msg->topicId, msg->topicType, msg->callback );


	while ( msg->retryCount < MQTTSN_RETRY_COUNT )
	{
		Connect();
		LoRaLinkStatus_t stat = WriteMsg( buf );

		DLOG("Send %s msgId: %c%04x\r\n", GetMsgType( buf[1] ), buf[2] & MQTTSN_FLAG_DUP ? '+' : ' ', msg->msgId );

		msg->retryCount++;

		if ( stat == LORALINK_STATUS_OK )
		{
			if ( GetMessage( MQTTSN_TIMEOUT_MS ) > 0 )
			{
				RestartPingRequestTimer();
				return MQTTSN_STATE_OK;
			}
		}

	}
	return MQTTSN_STATE_RETRY_OUT;
}

