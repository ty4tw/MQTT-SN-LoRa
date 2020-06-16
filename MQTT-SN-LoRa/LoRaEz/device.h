/*!
 * \file      device.h
 *
 * \brief     LoRaDevice general functions implementation
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
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <stdint.h>

#include "utilities.h"

#include "adc.h"
#include "device-config.h"
#include "i2c.h"
#include "spi.h"
#include "uart.h"

/*!
 * Possible power sources
 */
enum DevicePowerSources
{
    USB_POWER = 0,
    BATTERY_POWER,
};

/*!
 * \brief Setup Baudrate of UART.
 */
void DeviceSetBaudeRate(uint32_t baudrate);

/*!
 * \brief Initializes the UART2.
 */
void DeviceUartInit(void);

/*!
 * \brief Initializes the mcu.
 */
void DeviceInitMcu( void );

/*!
 * \brief Resets the mcu.
 */
void DeviceResetMcu( void );

/*!
 * \brief Initializes the boards peripherals.
 */
void DeviceInitPeriph( void );

/*!
 * \brief De-initializes the target board peripherals to decrease power
 *        consumption.
 */
void DeviceDeInitMcu( void );

/*!
 * \brief Gets the current potentiometer level value
 *
 * \retval value  Potentiometer level ( value in percent )
 */
uint8_t DeviceGetPotiLevel( void );

/*!
 * \brief Measure the Battery voltage
 *
 * \retval value  battery voltage in volts
 */
uint32_t DeviceGetBatteryVoltage( void );

/*!
 * \brief Get the current battery level
 *
 * \retval value  battery level [  0: USB,
 *                                 1: Min level,
 *                                 x: level
 *                               254: fully charged,
 *                               255: Error]
 */
uint8_t DeviceGetBatteryLevel( void );

/*!
 * Returns a pseudo random seed generated using the MCU Unique ID
 *
 * \retval seed Generated pseudo random seed
 */
uint32_t DeviceGetRandomSeed( void );

/*!
 * \brief Gets the board 64 bits unique ID
 *
 * \param [IN] id Pointer to an array that will contain the Unique ID
 */
void DeviceGetUniqueId( uint8_t *id );

/*!
 * \brief Manages the entry into ARM cortex deep-sleep mode
 */
void DeviceLowPowerHandler( void );

/*!
 * \brief Get the board power source
 *
 * \retval value  power source [0: USB_POWER, 1: BATTERY_POWER]
 */
uint8_t GetDevicePowerSource( void );

bool DeviceIsUartsBusy(void);

uint16_t DeviceMeasureVdd(void);
uint16_t DeviceAdcMeasureVoltage(uint32_t channel);
uint16_t DeviceAdcMeasureValue(uint32_t channel);

void DeviceUart1Init(uint32_t baudrate);

I2c_t* DeviceGetI2c(void);
Spi_t* DeviceGetSpi(void);
Adc_t* DeviceGetAdc(void);
Gpio_t* DeviceGetInt0(void);
Gpio_t* DeviceGetInt1(void);

uint8_t DeviceGetChar(uint16_t secs);

#endif // __DEVICE_H__
