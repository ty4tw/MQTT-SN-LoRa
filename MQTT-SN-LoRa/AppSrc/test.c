/*
 * test.c
 *
 *  Created on: 2020/06/02
 *      Author: tomoaki
 */
#include "LoRaEz.h"

#define RX_CH_DEVICE    40
#define TX_CH_DEVICE    41
#define SF_VALUE        SF_9
#define POWER           13
#define CRYPTOKEY      { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 }
void start(void)
{
	uint8_t key[] = CRYPTOKEY;
	SetUartBaudrate(115200);
	LoRaLinkCryptoSetKey(key);
//	LoRaLinkModemProcess( LORALINK_MODEM_TX, RX_CH_DEVICE, TX_CH_DEVICE, SF_VALUE, POWER );
	LoRaLinkModemProcess( LORALINK_MODEM_RX, TX_CH_DEVICE, RX_CH_DEVICE, SF_VALUE, POWER );
}

