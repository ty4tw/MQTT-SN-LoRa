/***************************************************************************************
 *
 * MQTTSNSubscribe.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#ifndef MQTTSNSUBSCRIB_H_
#define MQTTSNSUBSCRIB_H_

#include "MQTTSNTopic.h"



void OnConnect( void );
void SubscribeByName( uint8_t* topicName, TopicCallback onPublish, MQTTSNQos_t qos );
void SubscribeById( uint16_t topicId, uint8_t topicType, TopicCallback onPublish, MQTTSNQos_t qos );
void UnsubscribeByName( uint8_t* topicName );
void UnsubscribeById( uint16_t topicId, uint8_t topicType );
void GetSubscribeResponce( uint8_t* msg );
//bool IsSubscribeDone( void );
void ResponceSubscribe( uint8_t* resp );
//bool ChecktSubscribeRetryOut( void );

#endif /* MQTTSNSUBSCRIB_H_ */
