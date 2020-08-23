/***************************************************************************************
 *
 * MQTTSNPublish.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "MQTTSNClient.h"
#include "MQTTSNPublish.h"
#include "MQTTSNRegister.h"
#include "utilities.h"
#include "systime.h"
#include "TaskMgmt.h"

#define TOPICID_IS_SUSPEND  0
#define TOPICID_IS_READY    1
#define WAIT_PUBACK         2
#define WAIT_PUBREC         3
#define WAIT_PUBREL         4
#define WAIT_PUBCOMP        5
#define WAIT_NORESP         6

#define MQTTSN_FLAG_TOPIC_TYPE 0x03

typedef struct PubElement{
    uint16_t  msgId;
    uint16_t  topicId;
    uint8_t   topicName[ MQTTSN_MAX_TOPIC_LEN + 1 ];
    uint8_t*  payload;
    uint16_t  payloadlen;
    void      (*callback)(void);
    uint8_t   retryCount;
    uint8_t   flag;
    MQTTSNQos_t   qos;
    uint8_t   topicType;
    uint8_t   status;  // 0:SUSPEND, 1:READY
} MQTTSNPublish_t;

MQTTSNPublish_t PublishMsg = { 0 };

const char* NULLCHAR = "";


static MQTTSNState_t publish( uint8_t* topicName, uint16_t topicId, uint8_t* rowdata, uint8_t len, MQTTSNQos_t qos, uint8_t topicType, bool retain);

static MQTTSNState_t sendPublishMsg( MQTTSNPublish_t* msg );
static  LoRaLinkStatus_t sendPublish( MQTTSNPublish_t* msg );
void SendPubAck( uint16_t topicId, uint16_t msgId, uint8_t rc );
void SendPubRel( uint16_t msgId );

static void resetPublishMsg( MQTTSNPublish_t* msg )
{
	memset1( (uint8_t*)msg, 0, sizeof( MQTTSNPublish_t ) );
}

MQTTSNState_t PublishByName( uint8_t* topicName, Payload_t* payload, MQTTSNQos_t qos, bool retain)
{
	return PublishRowdataByName( topicName, GetPL_RowData( payload ), GetRowdataLength( payload ), qos, retain );
}


MQTTSNState_t PublishRowdataByName( uint8_t* topicName, uint8_t* rowdata, uint8_t len, MQTTSNQos_t qos, bool retain)
{
	uint8_t topicType = MQTTSN_TOPIC_TYPE_NORMAL;
	uint8_t topiclen = strlen ( (const char*)topicName );

	if ( topiclen == 2 )
	{
		topicType = MQTTSN_TOPIC_TYPE_SHORT;
	}
	return publish( topicName, 0, rowdata, len, qos, topicType, retain );
}

static MQTTSNState_t publish( uint8_t* topicName, uint16_t topicId, uint8_t* rowdata, uint8_t len, MQTTSNQos_t qos, uint8_t topicType, bool retain)
{
	uint16_t tid = 0;
	resetPublishMsg( &PublishMsg );

	if ( topicType == MQTTSN_TOPIC_TYPE_SHORT )
	{
		topicId = *topicName << 8;
		topicId |= *(topicName + 1);
	}

	if ( qos > QOS_0 )
	{
		PublishMsg.msgId = GetNextMsgId();
	}

	if ( topicName != NULL && topicType != MQTTSN_TOPIC_TYPE_SHORT )
	{
		MQTTSNTopic_t* topic = GetTopicByName( topicName );

		if ( topic != NULL )
		{
			tid = topic->TopicId;
		}
	}
	else
	{
		if ( topicId > 0 )
		{
			tid = topicId;
		}
	}

	if ( topicName != NULL && topicType != MQTTSN_TOPIC_TYPE_SHORT )
	{
		memcpy1( PublishMsg.topicName, topicName, strlen( (const char*)topicName ) );
	}

	PublishMsg.topicId = tid;
	PublishMsg.payload = rowdata;
	PublishMsg.payloadlen = len;
	PublishMsg.qos = qos;
	PublishMsg.topicType = topicType;
	PublishMsg.flag = qos ;
	PublishMsg.flag = PublishMsg.flag | ( retain << 4 );
	PublishMsg.flag = PublishMsg.flag | topicType;

	if ( tid > 0 )
	{
		PublishMsg.status = TOPICID_IS_READY;
		return sendPublishMsg( &PublishMsg );
	}
	else
	{
		PublishMsg.status = TOPICID_IS_SUSPEND;
		return RegisterTopic( topicName );
	}
}

MQTTSNState_t PublishRowdataByPredefinedId(uint16_t topicId, uint8_t* rowdata, uint8_t len, MQTTSNQos_t qos, bool retain)
{
	return publish( 0, topicId, rowdata, len, qos, MQTTSN_TOPIC_TYPE_PREDEFINED, retain );
}


void ResponcePublish( uint8_t* msg, uint8_t msglen )
{
	if ( msg == NULL )
	{
		return;
	}

	if (msg[0] == MQTTSN_TYPE_PUBACK)
	{
		uint16_t msgId = getUint16( msg + 3 );

		if ( PublishMsg.msgId != msgId )
		{
			return;
		}
		if (msg[5] == MQTTSN_RC_ACCEPTED)
		{
			if ( PublishMsg.status == WAIT_PUBACK )
			{
				resetPublishMsg( &PublishMsg );
			}
		}
		else if (msg[5] == MQTTSN_RC_REJECTED_INVALID_TOPIC_ID)
		{
			PublishMsg.status = TOPICID_IS_SUSPEND;
			PublishMsg.topicId = 0;
			PublishMsg.retryCount = MQTTSN_RETRY_COUNT;
			RegisterTopic( PublishMsg.topicName );
		}
	}
	else if (msg[0] == MQTTSN_TYPE_PUBREC)
	{
		uint16_t msgId = getUint16( msg + 1 );

		if ( PublishMsg.msgId == msgId )
		{
			return;
		}
		if ( PublishMsg.status == WAIT_PUBREC || PublishMsg.status == WAIT_PUBCOMP )
		{
			SendPubRel( msgId );
			PublishMsg.status = WAIT_PUBCOMP;
		}
	}
	else if ( msg[0] == MQTTSN_TYPE_PUBCOMP )
	{
		uint16_t msgId = getUint16( msg + 1 );

		if ( PublishMsg.msgId == msgId )
		{
			return;
		}

		if (PublishMsg.status == WAIT_PUBCOMP )
		{
			resetPublishMsg( &PublishMsg );
		}
	}
}


void SendPublishSuspend( uint8_t* topicName, uint16_t topicId, uint8_t topicType)
{
	if ( strcmp( (const char*)PublishMsg.topicName, (const char*)topicName) == 0 && PublishMsg.status == TOPICID_IS_SUSPEND)
	{
		PublishMsg.topicId = topicId;
		PublishMsg.flag |= topicType & MQTTSN_FLAG_TOPIC_TYPE;
		PublishMsg.status = TOPICID_IS_READY;
		sendPublishMsg( &PublishMsg );
	}
}

void SendPubAck( uint16_t topicId, uint16_t msgId, uint8_t rc )
{
	uint8_t msg[7];
	msg[0] = 7;
	msg[1] = MQTTSN_TYPE_PUBACK;
	setUint16(msg + 2, topicId);
	setUint16(msg + 4, msgId);
	msg[6] = rc;

	DLOG("Send %s msgId: %d\r\n", "PUBACK" , msgId );
	WriteMsg(msg);
}

void SendPubRel( uint16_t msgId)
{
	uint8_t buf[4];
	buf[0] = 4;
	buf[1] = MQTTSN_TYPE_PUBREL;
	setUint16( buf + 2, msgId );

	DLOG("Send %s msgId: %d\r\n", "PUBREL" , msgId );
	WriteMsg( buf );
}


static MQTTSNState_t sendPublishMsg( MQTTSNPublish_t* msg )
{

	while ( msg->retryCount < MQTTSN_RETRY_COUNT )
	{
		LoRaLinkStatus_t stat = sendPublish( msg );

		if ( stat == LORALINK_STATUS_OK )
		{
			if ( GetMessage( MQTTSN_TIMEOUT_MS ) > 0 )
			{
				return MQTTSN_STATE_OK;
			}
		}
	}
	return MQTTSN_STATE_RETRY_OUT;
}

static  LoRaLinkStatus_t sendPublish( MQTTSNPublish_t* msg )
{
	uint8_t buf[MQTTSN_MAX_MSG_LENGTH + 1];
	LoRaLinkStatus_t stat = LORALINK_STATUS_ERROR;

	if ( msg ==  NULL )
	{
		return LORALINK_STATUS_PARAMETER_INVALID;
	}

	if ( msg->qos != QOS_M1)
	{
		Connect();
	}


	if ( msg->retryCount == 0 )
	{

		buf[0] = (uint8_t)msg->payloadlen + 7;
		buf[1] = MQTTSN_TYPE_PUBLISH;

		setUint16( buf + 3, msg->topicId );
		setUint16( buf + 5, msg->msgId );
		memcpy1( buf + 7, msg->payload, msg->payloadlen);
	}
	else
	{
		msg->flag |= MQTTSN_FLAG_DUP;
	}

	buf[2] = msg->flag;

	stat =  WriteMsg( buf );
	msg->retryCount++;

	if ( stat != LORALINK_STATUS_OK )
	{
		return stat;
	}

	RestartPingRequestTimer( );

	DLOG("Send %s msgId: %c%04x\r\n", "PUBLISH", buf[2] & MQTTSN_FLAG_DUP ? '+' : ' ', msg->msgId );

	if ( msg->qos == QOS_0 || msg->qos == QOS_M1 )
	{
		msg->status = WAIT_NORESP;
		return stat;
	}
	else if ( msg->qos == QOS_1 )
	{
		msg->status = WAIT_PUBACK;
	}
	else if ( msg->qos == QOS_2 )
	{
		msg->status = WAIT_PUBREC;
	}

//	msg->retryCount++;

	return stat;
}

void Published( uint8_t* msg, uint8_t msglen )
{

	uint16_t topicId = 0;
	uint8_t  topicType = msg[2] & 0x03;

	if ( topicType == MQTTSN_TOPIC_TYPE_SHORT )
	{
		topicId = *(msg + 3) << 8;
		topicId |= *( msg + 4 );
	}
	else
	{
		topicId = getUint16( msg + 3 );
	}

	Payload_t payload;
	SetRowdataToPayload( &payload, msg + 7, msglen - 7 );

	if ( msg[2] & MQTTSN_FLAG_QOS_1 )
	{
		SendPubAck( topicId, getUint16( msg + 5) , MQTTSN_RC_ACCEPTED );
	}

	MQTTSNTopicExecCallback( topicId, topicType, &payload );
}
