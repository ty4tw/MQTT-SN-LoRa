/***************************************************************************************
 *
 * MQTTSNTopic.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#ifndef MQTTSNTOPIC_H_
#define MQTTSNTOPIC_H_

#include "MQTTSNDefines.h"
#include "Payload.h"

#define MQTTSN_TOPIC_MULTI_WILDCARD   1
#define MQTTSN_TOPIC_SINGLE_WILDCARD  2


typedef struct Topic
{
	uint8_t TopicType;
	uint16_t TopicId;
	uint8_t TopicName[ MQTTSN_MAX_TOPIC_LEN + 1 ];
	TopicCallback Callback;
	bool malocFlg;
	struct Topic* Next;
	struct Topic* Prev;
}MQTTSNTopic_t;

typedef struct
{
	MQTTSNTopic_t* Head;
	MQTTSNTopic_t* Tail;
}MQTTSNTopicTable_t;


void MQTTSNTopicExecCallback( uint16_t  topicId, uint8_t topicType, Payload_t* payload );

MQTTSNTopic_t* MQTTSNTopicAdd( uint8_t* topicName, uint16_t id, uint8_t type, TopicCallback callback );

MQTTSNTopic_t*   GetTopicByName( uint8_t* topic );
MQTTSNTopic_t*   GetTopicById( uint16_t topicId, uint8_t topicType );

void SetTopicId( uint8_t* topic, uint16_t id, uint8_t topicType );
void ClearTopicTable( void );
void removeTopic( uint16_t topicId, uint8_t type );
MQTTSNTopic_t* GetTopicMatch( uint8_t* topicName );
bool TopicIsMatch( MQTTSNTopic_t* topic, uint8_t* topicName );

//bool SetCallbackByName( uint8_t* topic, TopicCallback callback );
//bool SetCallbackById( uint16_t topicId, uint8_t type, TopicCallback callback );

#endif /* MQTTSNTOPIC_H_ */
