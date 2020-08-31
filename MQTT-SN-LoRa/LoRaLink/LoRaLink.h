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


/*!
 * Defines the LoRaMac radio events status
 */
typedef union uLoRaMacRadioEvents
{
    uint32_t Value;
    struct sEvents
    {
        uint32_t RxTimeout : 1;
        uint32_t RxError   : 1;
        uint32_t TxTimeout : 1;
        uint32_t RxDone    : 1;
        uint32_t TxDone    : 1;
        uint32_t TxDelaied : 1;
    }Events;
}LoRaLinkRadioEvents_t;

/*!
 * Initialize LoRaLink
 */
void LoRaLinkInitialize(void);

/*!
 * Setup Device Parameters
 *
 * \param [IN] key      Encription key
 * \param [IN] panId    Pan ID
 * \param [IN] devTxCh  Uplink   channel
 * \param [IN] devRxCh  Downlink channel
 * \param [IN] sfValue  Spreading Factor
 * \param [IN] power    Output power in dBm
 * \retval value    LoRaLinkStatus
 */
LoRaLinkStatus_t LoRaLinkDeviceInit( uint8_t* key, uint16_t panId, uint8_t devAddr, uint8_t syncWord, uint8_t uplinkCh, uint8_t dwnlinkCh, LoRaLinkSf_t sfValue, int8_t power );
/*!
 * Gateway Modem Process
 *
 * \param [IN] key      Encription key
 * \param [IN] panId    Pan ID
 * \param [IN] devTxCh  Uplink   channel
 * \param [IN] devRxCh  Downlink channel
 * \param [IN] sfValue  Spreading Factor
 * \param [IN] power    Output power in dBm
 * \retval value    LoRaLinkStatus
 */
LoRaLinkStatus_t LoRaLinkUart( uint8_t* key, uint16_t panId, uint8_t devAddr, LoRaLinkUartType_t devType, uint8_t syncWord, uint8_t devTxCh, uint8_t devRxCh, LoRaLinkSf_t sfValue );
/*!
 * Receive LoRaLink Packet
 *
 * \param [OUT] pkt Received packet pointer
 * \param [IN]  timeout  Receive time out value in ms
 * \retval value    LoRaLinkStatus
 */
LoRaLinkStatus_t LoRaLinkRecvPacket(LoRaLinkPacket_t* pkt, uint32_t timeout);
/*!
 * Send Payload
 *
 * \param [IN] destAddr     Destination address
 * \param [IN] payloadType  Payload Type
 * \param [IN] buffer       Payload data pointer
 * \param [IN] buffLen      Payload length
 * \param [IN] timeout      Send time out value in ms
 * \retval value    LoRaLinkStatus
 */
LoRaLinkStatus_t LoRaLinkSend( uint8_t destAddr, uint8_t payloadType, uint8_t* buffer, uint8_t buffLen, uint32_t timeout );


/*!
 *
 */
LoRaLinkStatus_t LoRaLinkSerializeData(LoRaLinkPacket_t* pkt);
/*!
 *
 */
LoRaLinkStatus_t LoRaLinkDeserializeData(LoRaLinkPacket_t* pkt, RxDoneParams_t* rxData);
/*!
 *	Setup PanID & Device address
 *
 * \param [IN] panId    PanId
 * \param [IN] devAddr  Device address
 * \retval value    LoRaLinkStatus
 */
LoRaLinkStatus_t LoRaLinkSetDeviceId( uint16_t panId, uint8_t devAddr );
/*!
 * Get Tx data from LoraLinkPacket and setup
 *
 * param [IN] LoRaLinkPkt    PacketData to be sent
 */
void LoRaLinkSetTxData( LoRaLinkPacket_t* LoRaLinkPkt );
/*!
 * Get Max available payload length
 *
 * \retval value  Payload length
 */
uint8_t LoRaLinkGetMaxPayloadLength( void );

uint8_t LoRaLinkGetSourceAddr( void );

uint16_t LoRaLinkGetPanId( void );

LoRaLinkPacket_t* LoRaLinkClearPacket( LoRaLinkPacket_t* pkt );

int16_t LoRaLinkGetRssi( void );
int8_t LoRaLinkGetSnr( void );


#endif /* LORALINK_H_ */
