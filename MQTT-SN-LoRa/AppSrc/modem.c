/*
 * modem.c
 *
 *  Created on: 2020/06/23
 *      Author: tomoaki
 */
#include "commission.h"
#include "LoRaEz.h"
#include "LoRaLink.h"
#include "LoRaLinkApi.h"

void start(void)
{
	SetUartBaudrate(115200);

	LoRaLinkUartType_t type = LORALINK_UART_RX;
//	LoRaLinkUartType_t type = LORALINK_UART_TX;


	LoRaLinkUart( CRYPTO_KEY, PANID, UART_DEVADDR, type, SYNCWORD, UPLINK_CH, DWNLINK_CH, SF_VALUE, POWER_IN_DBM );
}

