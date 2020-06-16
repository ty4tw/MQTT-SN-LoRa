/**************************************************************************************
 *
 * LoRaLink.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#ifndef LORALINK_H_
#define LORALINK_H_

#include "LoRaLinkTypes.h"
#include "systime.h"
#include "timer.h"
#include "radio.h"





void LoRaLinkInitialize(void);

LoRaLinkStatus_t LoRaLinkModemProcess( LoRaLinkDeviceType_t devType, uint8_t devRxCh, uint8_t devTxCh, LoRaLinkSf_t sfValue, int8_t power );
LoRaLinkStatus_t LoRaLinkDeviceInit( uint8_t devRxCh, uint8_t devTxCh, LoRaLinkSf_t sfValue, int8_t power );
LoRaLinkStatus_t LoRaLinkDeviceExecution(void);

LoRaLinkStatus_t LoRaLinkSerializeData(LoRaLinkPacket_t* pkt);
LoRaLinkStatus_t LoRaLinkDeserializeData(LoRaLinkPacket_t* pkt, RxDoneParams_t* rxData);
//LoRaLinkStatus_t LoRaLinkSend(uint8_t* buffer, uint8_t bufferLen, uint32_t timeout);
//LoRaLinkStatus_t LoRaLinkRecv(uint8_t* buffer, uint8_t bufferLen, uint32_t timeout);

void LoRaLinkSetTxData( LoRaLinkPacket_t* LoRaLinkPkt );

#endif /* LORALINK_H_ */
