/***************************************************************************************
 *
 * MQTTSNRegister.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#ifndef MQTTSNPUBLISH_H_
#define MQTTSNPUBLISH_H_

#include "MQTTSNTopic.h"
#include "Payload.h"


void PublishByName( uint8_t* topicName, Payload_t* payload, uint8_t qos, bool retain );
void PublishRowdataByName( uint8_t* topicName, uint8_t* rowdata, uint8_t len, uint8_t qos, bool retain );
void PublishRowdataByPredefinedId( uint16_t topicId, uint8_t* rowdata, uint8_t len, uint8_t qos, bool retain );
void ResponcePublish( uint8_t* msg, uint8_t msglen );
void Published( uint8_t* msg, uint8_t msglen );
void SendPublishSuspend( uint8_t* topicName, uint16_t topicId, uint8_t topicType );
bool IsPublishDone( void );
bool IsMaxFlight( void );



#endif /* MQTTSNPUBLISH_H_ */
