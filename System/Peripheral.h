/*!
 * \file      Peripheral.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#ifndef _PERIPHERAL_H_
#define _PERIPHERAL_H_

#include <stdio.h>
#include <string.h>
#include "utilities.h"

#include "adc.h"
#include "device.h"
#include "device-config.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"

#if defined(ABZ78_R)

typedef enum {
	GP0,                       // PB_5
	GP1,                       // PB_6
	GP2,                       // PB_7
	GP3,                       // PB_8
	GP4,                       // PB_9
	GP5,                       // PA_5
	GP6,                       // PA_11
} PortNo_t;

#else

typedef enum {
	GP0,                       // PA_0
	GP1,                       // PB_5
	GP2,                       // PB_6
	GP3,                       // PB_7
	GP4,                       // PB_8
	GP5,                       // PB_9
	GP6,                       // PA_5
	GP7,                       // PA_11
} PortNo_t;
#endif


typedef enum {
	CH0,
	CH1
} AdcCh_t;

/*!
 * \brief Configures the UART object and MCU peripheral
 *
 * \param [IN] baudrate     UART baudrate
 */
void SetUartBaudrate( uint32_t baudrate );

/*!
 * \brief Write data buffer to the I2C device
 *
 * \param [IN] deviceAddr       device address
 * \param [IN] addr             data address
 * \param [IN] buffer           data buffer to write
 * \param [IN] size             number of bytes to write
 */
uint8_t WriteI2c( uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size );

/*!
 * \brief Read data buffer from the I2C device
 *
 * \param [IN] deviceAddr       device address
 * \param [IN] addr             data address
 * \param [OUT] buffer          data buffer to read
 * \param [IN] size             number of data bytes to read
 */
uint8_t ReadI2c( uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size );

/*!
 * \brief Sends outData and receives inData
 *
 * \param [IN] outData Byte to be sent
 * \retval inData      Received byte.
 */
uint16_t ReadWriteSpi( uint16_t outData );

/*!
 * \brief Get the voltage of Pin15
 *
 * \param [IN] ch   ADC channel  CH0 / CH1
 * \retval value    millVolt
 *
 */
uint16_t ReadAdcVoltage( AdcCh_t ch );

/*!
 * \brief Get the value of Pin15
 *
 * \param [IN] ch   ADC channel  CH0 / CH1
 * \retval value    millVolt
 *
 */
uint16_t ReadAdc( AdcCh_t ch );

/*!
 * \brief Get the voltage of Vdd
 * *
 * \retval value    millVolt
 *
 */
uint16_t ReadVdd( void );

/*!
 * \brief Initializes the given GPIO object
 *
 * \param [IN] no     PortNo
 * \param [IN] mode   Pin mode [PIN_INPUT, PIN_OUTPUT, PIN_ANALOGIC]
 * \param [IN] config Pin config [PIN_PUSH_PULL, PIN_OPEN_DRAIN]
 * \param [IN] type   Pin type [PIN_NO_PULL, PIN_PULL_UP, PIN_PULL_DOWN]
 */
void InitGpio( PortNo_t no, PinModes mode, PinConfigs config, PinTypes type );
/*!
 * \brief Writes the given value to the GPIO output
 *
 * \param [IN] no    Gpio PortNo
 * \param [IN] value New GPIO output value
 */
void WriteGpio( PortNo_t no, uint32_t value );

/*!
 * \brief Toggle the value to the GPIO output
 *
 * \param [IN] no    Gpio PortNo
 */
void ToggleGpio( PortNo_t no );

/*!
 * \brief Reads the current GPIO input value
 *
 * \param [IN] no  Gpio PortNo
 * \retval value   Current GPIO input value
 */
uint32_t ReadGpio( PortNo_t no );

/*!
 * \brief Reads the current GPIO input value
 *
 * \retval value   RSSI
 */
int16_t GetRssi( void );

/*!
 * \brief Reads the current GPIO input value
 *
 * \retval value   SNR
 */
int8_t GetSnr( void );

/*!
 * \brief Get Free RAM size
 * *
 * \retval value    Free RAM size in bytes
 *
 */
int getFreeRam(void);

#endif /* _PERIPHERAL_H_ */
