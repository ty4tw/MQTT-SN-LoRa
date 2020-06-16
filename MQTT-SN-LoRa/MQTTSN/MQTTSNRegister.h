/***************************************************************************************
 *
 * MQTTSNRegister.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#ifndef MQTTSNREGISTER_H_
#define MQTTSNREGISTER_H_

#include "MQTTSNTopic.h"
#include "MQTTSNClient.h"


MQTTSNState_t RegisterTopic( uint8_t* topicName );
MQTTSNState_t ResponceRegAck( uint16_t msgId, uint16_t topicId );
void ResponceRegister( uint8_t* msg, uint16_t msglen );
bool IsRegisterDone(void);
uint8_t checkTimeout();
uint8_t* getRegTopic( uint16_t msgId );


#endif /* MQTTSNREGISTER_H_ */
