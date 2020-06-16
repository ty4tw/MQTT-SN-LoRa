 /**************************************************************************************
 *
 * LoRaLink.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include "LoRaLinkApi.h"
#include "LoRaLinkTypes.h"
#include "LoRaLinkCrypto.h"
#include "LoRaLink.h"
#include "timer.h"
#include "utilities.h"
#include "delay.h"
#include "uart.h"

#define FRAME_DLMT   0x7E
#define ESCAPE       0x7D
#define XON          0x11
#define XOFF         0x13
#define PAD          0x20

static uint8_t UartGetByte( uint8_t* buf );
static void UartPutByte( uint8_t c );
static void LoRaLinkApiWrite( LoRaLinkApiType_t api, LoRaLinkPacket_t* pkt );


void LoRaLinkApiGetRxData( LoRaLinkPacket_t* pkt, RxDoneParams_t* rxDonePara )
{
	uint8_t* pos = rxDonePara->Payload;

	pkt->Buffer = pos;
	pkt->BufSize = rxDonePara->Size;

	pkt->PanId = getUint16(pos);
    pos += 2;
    pkt->DestAddr = *pos++;
    pkt->SourceAddr = *pos++;
    pkt->FRMPayload = pos;
    pkt->FRMPayloadSize = rxDonePara->Size - LORALINK_HDR_LEN - LORALINK_MIC_LEN;
	pkt->Rssi = rxDonePara->Rssi;
	pkt->Snr = rxDonePara->Snr;

	pos += pkt->FRMPayloadSize;
	pkt->MIC = getUint32(pos);
}

static void LoRaLinkApiWrite( LoRaLinkApiType_t api, LoRaLinkPacket_t* pkt )
{
	uint8_t buf[4] = { 0 };
	uint8_t* pos = 0;
	uint8_t chks = 0;
	uint16_t len = pkt->FRMPayloadSize + LORALINK_HDR_LEN + LORALINK_MIC_LEN + 2;  // 1:CRC

	UartPutByte(FRAME_DLMT);

	setUint16(buf,len);
	UartPutByte(buf[0]);
	UartPutByte(buf[1]);

	UartPutByte(api);

	setUint16(buf, pkt->PanId);
	UartPutByte(buf[0]);
	UartPutByte(buf[1]);
	chks += buf[0] + buf[1];

	UartPutByte( pkt->DestAddr);
	chks += pkt->DestAddr;
	UartPutByte(pkt->SourceAddr);
	chks += pkt->SourceAddr;

	setUint16(buf, pkt->Rssi);
	UartPutByte(buf[0]);
	UartPutByte(buf[1]);
	chks += buf[0] + buf[1];

	setUint16(buf, pkt->Snr);
	UartPutByte(buf[0]);
	UartPutByte(buf[1]);
	chks += buf[0] + buf[1];

	pos = pkt->FRMPayload;
	for( uint8_t i = 0; i < pkt->FRMPayloadSize; i++ )
	{
		UartPutByte(*pos);
		chks += *pos++;
	}

	setUint32( buf, pkt->MIC);
	chks += buf[0] + buf[1] + buf[2] + buf[3];

	UartPutByte(0xff - chks);  // CRC
}

void LoRaLinkApiPutRecvData(LoRaLinkPacket_t* pkt)
{
	LoRaLinkApiWrite(API_RX_DATA, pkt);
}

void LoRaLinkApiPutSendResp(LoRaLinkPacket_t* pkt)
{
	LoRaLinkApiWrite(API_TX_RESP, pkt);
}


void LoRaLinkApiSetTxData( LoRaLinkPacket_t* LoRaLinkPkt, LoRaLinkApi_t* LoRaLinkApi )
{
	LoRaLinkPkt->PanId = LoRaLinkApi->PanId;
	LoRaLinkPkt->DestAddr = LoRaLinkApi->DestinationAddr;
	LoRaLinkPkt->SourceAddr = LoRaLinkApi->SourceAddr;
	LoRaLinkPkt->FRMPayload = LoRaLinkApi->Payload;
	LoRaLinkPkt->FRMPayloadSize = LoRaLinkApi->PayloadLen;

	LoRaLinkSetTxData(LoRaLinkPkt);
}


bool LoRaLinkApiRead(LoRaLinkApi_t* api, LoRaLinkApiReadParameters_t* para)
{
	uint8_t byte = 0;
	uint16_t val = 0;

	while ( UartGetByte(&byte) == 1 )
	{
		if ( byte == FRAME_DLMT )
		{
			para->apipos= 1;
			para->Error = true;
			para->Available = false;
			continue;
		}

		switch ( para->apipos )
		{
		case 0:
			break;

		case 1:
			val = (uint16_t)byte;
			api->PayloadLen = val << 8;
			break;

		case 2:
			api->PayloadLen += byte;
			break;

		case 3:
			api->ApiType = byte;
			para->checksum = byte;
			break;

		case 4:
			val = (uint16_t)byte;
			api->PanId = val << 8;
			para->checksum += byte;
			break;

		case 5:
			api->PanId += byte;
			para->checksum += byte;
			break;

		case 6:
			api->DestinationAddr = byte;
			para->checksum += byte;
			break;

		case 7:
			api->SourceAddr = byte;
			para->checksum += byte;
			break;

		default:
			if ( para->apipos >= api->PayloadLen + 2 )
			{
				para->Error = ( (0xff - para->checksum) != byte );
				para->Available = true;
				api->PayloadLen -= 6;
				para->apipos = 0;
				return true;
			}
			else
			{
				para->checksum += byte;
				api->Payload[para->apipos - 8] = byte;
			}
			break;
		}

		para->apipos++;
	}
	return false;
}


static uint8_t UartGetByte( uint8_t* buf )
{
	if ( UartGetChar(buf) == 0 )
	{
		if ( *buf == ESCAPE )
		{
			DelayMs(1);
			UartGetChar(buf);
			*buf = PAD ^ *buf;
		}
		return 1;
	}
	return 0;
}

static void UartPutByte( uint8_t c )
{
	if ( c == FRAME_DLMT || c == ESCAPE || c == XON || c == XOFF )
	{
		UartPutChar( ESCAPE );
		UartPutChar( c ^ PAD );
	}
	else
	{
		UartPutChar( c );
	}
}
