 /**************************************************************************************
 *
 * commission.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#ifndef COMMISSION_H_
#define COMMISSION_H_
#include <stdint.h>

#define UPLINK_CH       40
#define DWNLINK_CH      41
#define SF_VALUE        SF_9
#define POWER_IN_DBM    13

#define PANID           0x0102
#define UART_DEVADDR    0xFE

#define SYNCWORD        0x55

uint8_t CRYPTO_KEY[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 };

#endif /* COMMISSION_H_ */
