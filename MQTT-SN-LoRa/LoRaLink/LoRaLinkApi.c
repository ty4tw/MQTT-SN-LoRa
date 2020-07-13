 /**************************************************************************************
 *
 * LoRaLinkApi.c
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
#include "aes.h"
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

extern uint8_t LoRaLinkGetSourceAddr(void);
extern uint16_t LoRaLinkGetPanId(void);

void LoRaLinkApiGetRxData( LoRaLinkPacket_t* pkt, RxDoneParams_t* rxDonePara )
{
	uint8_t* pos = rxDonePara->Payload;

	pkt->Buffer = pos;
	pkt->BufSize = rxDonePara->Size;

	pkt->PanId = getUint16(pos);
    pos += 2;
    pkt->DestAddr = *pos++;
    pkt->SourceAddr = *pos++;
    pkt->FRMPayloadType = *pos++;
    pkt->FRMPayload = pos;
    pkt->FRMPayloadSize = rxDonePara->Size - LORALINK_HDR_LEN - LORALINK_MIC_LEN;

	pkt->MIC = getUint32(pos + pkt->FRMPayloadSize);

    pkt->Rssi = rxDonePara->Rssi;
	pkt->Snr = rxDonePara->Snr;
}

void LoRaLinkApiWrite( LoRaLinkPacket_t* pkt )
{
	uint8_t buf[4] = { 0 };
	uint8_t* pos = 0;
	uint8_t chks = 0;
	uint16_t len = pkt->FRMPayloadSize + 7;  //  = DestAddr[1] + Rssi[2] + Snr[2] + PayloadType[1] + Crc[1]

	UartPutByte(FRAME_DLMT);

	setUint16(buf,len);
	UartPutByte(buf[0]);
	UartPutByte(buf[1]);

	UartPutByte(pkt->SourceAddr);
	chks = pkt->SourceAddr;

	setUint16(buf, pkt->Rssi);
	UartPutByte(buf[0]);
	UartPutByte(buf[1]);
	chks += buf[0] + buf[1];

	setUint16(buf, pkt->Snr);
	UartPutByte(buf[0]);
	UartPutByte(buf[1]);
	chks += buf[0] + buf[1];

	UartPutByte( pkt->FRMPayloadType);
	chks += pkt->FRMPayloadType;

	pos = pkt->FRMPayload;
	for( uint8_t i = 0; i < pkt->FRMPayloadSize; i++ )
	{
		UartPutByte(*pos);
		chks += *pos++;
	}

	UartPutByte(0xff - chks);  // CRC
}


void LoRaLinkApiSetTxData( LoRaLinkPacket_t* pkt, LoRaLinkApi_t* api )
{
	pkt->PanId = api->PanId;
	pkt->DestAddr = api->DestinationAddr;
	pkt->SourceAddr = api->SourceAddr;
	pkt->FRMPayloadType = api->PayloadType;
	pkt->FRMPayload = api->Payload;
	pkt->FRMPayloadSize = api->PayloadLen;

	LoRaLinkSetTxData(pkt);
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
			api->DestinationAddr = byte;
			para->checksum = byte;
			break;

		case 4:
			api->PayloadType = byte;
			para->checksum += byte;
			break;

		default:
			if ( para->apipos >= api->PayloadLen + 2 )    //  FRM_DEL + CRC = 2
			{
				para->Error = ( (0xff - para->checksum) != byte );
				para->Available = true;
				para->apipos = 0;
				para->checksum = 0;
				api->PayloadLen -= 3;   // 3 = DestAddr[1] + PlType[1] + Crc[1]
				api->SourceAddr = LoRaLinkGetSourceAddr();
				api->PanId = LoRaLinkGetPanId();
				return true;
			}
			else
			{
				api->Payload[para->apipos - 5] = byte;
				para->checksum += byte;
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
