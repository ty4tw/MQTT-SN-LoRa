/*
 * SetupDevices.c
 *
 *  Created on: 2020/07/07
 *      Author: tomoaki
 */
#include "commission.h"
#include "LoRaEz.h"

#define DEV_ADDR 0x04

#if defined( CLIENT )

MQTTSNConf_t conf =
{
	"LoRaCl",  //Prefix of ClientId (max 8digits)  ClientId: prefix + DevAddr (2digits)
	180,       //KeepAlive (seconds)
	false,     //Clean session
	"",        //WillTopic
	"",        //WillMessage
	QOS_0,     //WillQos
	false      //WillRetain
};

// Max topic length  DR2:9, DR3- : 50
uint8_t topic1[] = "test1";
uint8_t topic3[] = "ty";

// Max Payload length  DR2: 8  DR3: 50, DR4: 237  DR5: 237
uint8_t rowdata1[] = "12345";
uint8_t rowdata2[] = "56789";

bool retain = false;

void start(void)
{
	SetUartBaudrate(115200);
	printf("\r\n\r\nStart\r\n");

	LoRaLinkDeviceInit( CRYPTO_KEY, PANID, DEV_ADDR, SYNCWORD, UPLINK_CH, DWNLINK_CH, SF_VALUE, POWER_IN_DBM );
	MQTTSNClientInit( &conf );
	printf("ClientId: %s\r\n", GetClientId() );
	Connect();
}

void task1( void )
{
	printf( "%s Task1 start\r\n", SysTimeGetStrLocalTime(0) );
	Payload_t pl;
	ClearPayload( &pl );
	SetRowdataToPayload( &pl, rowdata2, 5 );

	MQTTSNState_t state = PublishByName( topic3, &pl, QOS_1, retain );
	printf( "State = %d\r\n", (int)state );
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
	printf( "Rssi : %d Snr : %d\r\n", GetRssi(), GetSnr() );
}



SUBSCRIBE_LIST =  // { topic, callback, QOS_0 - QOS_2 },
{
	SUB( topic1, on_publish, QOS_0 ),
	SUB( topic3, on_publish, QOS_0 ),

	END_OF_SUBSCRIBE_LIST
};

//void wakeup(void)
//{
//	printf("wake up \r\n");
//}
/*===============================================================*/

#elif defined( RXMODEM ) || defined( TXMODEM )

void start(void)
{
	LoRaLinkUartType_t type = LORALINK_UART_RX;

	SetUartBaudrate(115200);

#if defined( TXMODEM )
	type = LORALINK_UART_TX;
#endif

	LoRaLinkUart( CRYPTO_KEY, PANID, UART_DEVADDR, type, SYNCWORD, UPLINK_CH, DWNLINK_CH, SF_VALUE );
}

/*===============================================================*/

#else

#error  "TYPE=CLIENT or TYPE=RXMODEM or TYPE=TXMODEM,  One of them is required."
#endif
