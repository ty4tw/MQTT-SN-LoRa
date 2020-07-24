/*
 * test.c
 *
 *  Created on: 2020/07/07
 *      Author: tomoaki
 */
#include "commission.h"
#include "LoRaEz.h"

#define CLIENT


#if defined( CLIENT )

#ifndef DUMMY
MQTTSNConf_t conf =
{
	"cl",           //Prefix of ClientId
	180,          //KeepAlive (seconds)
	false,           //Clean session
	"",    //WillTopic
	"",  //WillMessage
	QOS_0,          //WillQos
	false           //WillRetain
};

uint8_t topic1[] = "test1";
uint8_t topic3[] = "ty";

uint8_t rowdata1[] = "12345";
uint8_t rowdata2[] = "56789";

bool retain = false;

void start(void)
{

	SetUartBaudrate(115200);
	printf("\r\n\r\nStart\r\n");

	LoRaLinkDeviceInit( CRYPTO_KEY, PANID, 0x04, SYNCWORD, UPLINK_CH, DWNLINK_CH, SF_VALUE, POWER_IN_DBM );
	MQTTSNClientInit( &conf );
}

void task1( void )
{
	printf( "Task1 start\r\n");
	Payload_t pl;
	ClearPayload( &pl );
	SetRowdataToPayload( &pl, rowdata2, 5 );

	PublishByName( topic3, &pl, QOS_1, retain );
	Disconnect(0);
	printf( "Task1 end\r\n");
}

TASK_LIST =
{
		TASK( task1, 0, 5 ),
		END_OF_TASK_LIST
};

void on_publish( Payload_t* payload )
{
	printf( "on publish\r\n" );

	for ( int i = 0; i < GetRowdataLength(payload); i++ )
	{
		printf( "%0X ", GetUint8(payload) );
	}
	printf("\r\n");
}



SUBSCRIBE_LIST =  // { topic, callback, QOS_0 - QOS_2 },
{
	SUB( topic1, on_publish, QOS_1 ),
	SUB( topic3, on_publish, QOS_1 ),

	END_OF_SUBSCRIBE_LIST
};
#endif

#elif defined( RXMODEM ) || defined( TXMODEM )


void start(void)
{
	SetUartBaudrate(115200);

#if defined( RXMODEM )
	LoRaLinkUart( CRYPTO_KEY, PANID, UART_DEVADDR, LORALINK_UART_RX, SYNCWORD, UPLINK_CH, DWNLINK_CH, SF_VALUE, POWER_IN_DBM );

#else
	LoRaLinkUart( CRYPTO_KEY, PANID, UART_DEVADDR, LORALINK_UART_TX, SYNCWORD, UPLINK_CH, DWNLINK_CH, SF_VALUE, POWER_IN_DBM );
#endif
}
#else

#error  "TYPE=CLIENT or TYPE=RXMODEM or TYPE=TXMODEM is required."
#endif
