/*!
 * \file      device-config.h
 *
 * \brief     Device configuration
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 *               ___ _____ _   ___ _  _____ ___  ___  ___ ___
 *              / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
 *              \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
 *              |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
 *              embedded.connectivity.solutions===============
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    Daniel Jaeckle ( STACKFORCE )
 *
 * \author    Johannes Bruder ( STACKFORCE )
 */
#ifndef __DEVICE_CONFIG_H__
#define __DEVICE_CONFIG_H__


/*!
* Device UART Baudrate
*/
#define UART_BAUDRATE                              115200

/*!
 * Defines the time required for the TCXO to wakeup [ms].
 */
#define DEVICE_TCXO_WAKEUP_TIME                      5

/*!
 * Device MCU pins definitions
 */
#define RADIO_RESET                                 PC_0

#define RADIO_MOSI                                  PA_7
#define RADIO_MISO                                  PA_6
#define RADIO_SCLK                                  PB_3

#define RADIO_NSS                                   PA_15

#define RADIO_DIO_0                                 PB_4
#define RADIO_DIO_1                                 PB_1
#define RADIO_DIO_2                                 PB_0
#define RADIO_DIO_3                                 PC_13
#define RADIO_DIO_4                                 PA_5
#define RADIO_DIO_5                                 PA_4

#define RADIO_TCXO_POWER                            PA_12

#define RADIO_ANT_SWITCH_RX                         PA_1
#define RADIO_ANT_SWITCH_TX_BOOST                   PC_1
#define RADIO_ANT_SWITCH_TX_RFO                     PC_2


#define INT_0                                       PA_8
#define INT_1                                       PA_11

#define OSC_LSE_IN                                  PC_14
#define OSC_LSE_OUT                                 PC_15

#define OSC_HSE_IN                                  PH_0
#define OSC_HSE_OUT                                 PH_1

#define SWCLK                                       PA_14
#define SWDIO                                       PA_13

#define I2C_SCL                                     PB_8
#define I2C_SDA                                     PB_9

#define UART1_TX                                    PA_9
#define UART1_RX                                    PA_10

#define ADC_0                                       PA_2
#define ADC_1                                       PA_3

#define SPI2_MOSI                                   PB_15
#define SPI2_MISO                                   PB_14
#define SPI2_SCLK                                   PB_13
#define SPI2_NSS                                    PB_12

// Debug pins definition.
#define RADIO_DBG_PIN_TX                            PB_13
#define RADIO_DBG_PIN_RX                            PB_14

#if defined(ABZ78_R)
// GPIO pins
#define GPIO_0   	                                PA_0
#define GPIO_1                                      PB_2
#define GPIO_2                                      PB_7
#define GPIO_3                                      PB_6
#define GPIO_4                                      PB_5
#else
#if defined(ABZ78)
// GPIO pins
#define GPIO_0                                      PA_4
#define GPIO_1                                      PA_0
#define GPIO_2                                      PB_2
#define GPIO_3                                      PB_7
#define GPIO_4                                      PB_6
#define GPIO_5                                      PB_5
#endif
#endif

#endif // __DEVICE_CONFIG_H__
