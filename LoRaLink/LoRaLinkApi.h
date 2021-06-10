/**************************************************************************************
 *
 * LoRaLinkApi.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#ifndef LORALINKAPI_H_
#define LORALINKAPI_H_

#include <stdbool.h>
#include "LoRaLinkTypes.h"


typedef struct
{
	bool Available;
	bool Error;
	bool Escape;
	uint16_t apipos;
	uint8_t checksum;
} LoRaLinkApiReadParameters_t;




bool LoRaLinkApiRead(LoRaLinkApi_t* api, LoRaLinkApiReadParameters_t* para);
void LoRaLinkApiSerializeData( LoRaLinkPacket_t* pkt );
void LoRaLinkApiGetRxData( LoRaLinkPacket_t* pkt, RxDoneParams_t* rxDonePara );

void LoRaLinkApiSetTxData( LoRaLinkPacket_t* pkt, LoRaLinkApi_t* LoRaLinkApi );
void LoRaLinkApiWrite( LoRaLinkPacket_t* pkt );

void LoRaLinkApiPutRecvData(LoRaLinkPacket_t* pkt);
void LoRaLinkApiPutSendResp(LoRaLinkPacket_t* pkt);

#endif /* LORALINKAPI_H_ */
