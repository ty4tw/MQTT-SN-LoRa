/*
 * MQTTSNDefines.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#ifndef MQTTSNDEFINES_H_
#define MQTTSNDEFINES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "Payload.h"


/*======================================
      MQTT-SN Defines
========================================*/
#define MQTTSN_TYPE_ADVERTISE     0x00
#define MQTTSN_TYPE_SEARCHGW      0x01
#define MQTTSN_TYPE_GWINFO        0x02
#define MQTTSN_TYPE_CONNECT       0x04
#define MQTTSN_TYPE_CONNACK       0x05
#define MQTTSN_TYPE_WILLTOPICREQ  0x06
#define MQTTSN_TYPE_WILLTOPIC     0x07
#define MQTTSN_TYPE_WILLMSGREQ    0x08
#define MQTTSN_TYPE_WILLMSG       0x09
#define MQTTSN_TYPE_REGISTER      0x0A
#define MQTTSN_TYPE_REGACK        0x0B
#define MQTTSN_TYPE_PUBLISH       0x0C
#define MQTTSN_TYPE_PUBACK        0x0D
#define MQTTSN_TYPE_PUBCOMP       0x0E
#define MQTTSN_TYPE_PUBREC        0x0F
#define MQTTSN_TYPE_PUBREL        0x10
#define MQTTSN_TYPE_SUBSCRIBE     0x12
#define MQTTSN_TYPE_SUBACK        0x13
#define MQTTSN_TYPE_UNSUBSCRIBE   0x14
#define MQTTSN_TYPE_UNSUBACK      0x15
#define MQTTSN_TYPE_PINGREQ       0x16
#define MQTTSN_TYPE_PINGRESP      0x17
#define MQTTSN_TYPE_DISCONNECT    0x18
#define MQTTSN_TYPE_WILLTOPICUPD  0x1A
#define MQTTSN_TYPE_WILLTOPICRESP 0x1B
#define MQTTSN_TYPE_WILLMSGUPD    0x1C
#define MQTTSN_TYPE_WILLMSGRESP   0x1D

#define MQTTSN_TOPIC_TYPE_NORMAL     0x00
#define MQTTSN_TOPIC_TYPE_PREDEFINED 0x01
#define MQTTSN_TOPIC_TYPE_SHORT      0x02
#define MQTTSN_TOPIC_TYPE_RESERVED   0x03

#define MQTTSN_FLAG_DUP     0x80
#define MQTTSN_FLAG_QOS_0   0x0
#define MQTTSN_FLAG_QOS_1   0x20
#define MQTTSN_FLAG_QOS_2   0x40
#define MQTTSN_FLAG_QOS_M1  0xc0
#define MQTTSN_FLAG_RETAIN  0x10
#define MQTTSN_FLAG_WILL    0x08
#define MQTTSN_FLAG_CLEAN   0x04

#define MQTTSN_PROTOCOL_ID  0x01
#define MQTTSN_HEADER_SIZE  2

#define MQTTSN_RC_ACCEPTED                  0x00
#define MQTTSN_RC_REJECTED_CONGESTION       0x01
#define MQTTSN_RC_REJECTED_INVALID_TOPIC_ID 0x02
#define MQTTSN_RC_REJECTED_NOT_SUPPORTED    0x03

/****************************************
      MQTT-SN Parameters
*****************************************/
#define MQTTSN_MAX_MSG_LENGTH  (245)
#define MQTTSN_MAX_PACKET_SIZE (245)
#define MQTTSN_MAX_TOPIC_LEN   (64)

#define MQTTSN_DEFAULT_KEEPALIVE_SEC (3600)     // 1H=3600sec
#define MQTTSN_DEFAULT_DURATION_SEC   (900)     // 15min=900sec
#define MQTTSN_TIMEOUT_MS           (10000)    // 10sec=10000ms
#define MQTTSN_RETRY_COUNT              (3)
/*======================================
  MACROs and structure for Application
=======================================*/
typedef enum
{
	QOS_0 =  0x00,
	QOS_1 =  0x20,
	QOS_2 =  0x40,
	QOS_M1=  0xc0,
}MQTTSNQos_t;

typedef struct
{
	char*  clientId;
	uint16_t keepAlive;
	bool     cleanSession;
	char*  willTopic;
	char*  willMsg;
	MQTTSNQos_t  willQos;
    bool     willRetain;
}MQTTSNConf_t;

typedef void (*TopicCallback)( Payload_t* );

typedef struct OnPublishList
{
	uint8_t* topicName;
	TopicCallback pubCallback;
	MQTTSNQos_t qos;
}OnPublishList_t;



#define SUBSCRIBE_LIST    OnPublishList_t theOnPublishList[]
#define SUB(...)          { __VA_ARGS__ }
#define END_OF_SUBSCRIBE_LIST { 0,0,0 }

#endif /* MQTTSNDEFINES_H_ */
