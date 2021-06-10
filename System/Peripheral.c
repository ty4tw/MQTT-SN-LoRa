/*!
 * \file      Peripheral.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "Peripheral.h"
#include "utilities.h"

#include "device.h"
#include "device-config.h"
#include "LoRaLink.h"

#if defined(ABZ78_R)
static Gpio_t EzGpios[7] = { 0 };
static PinNames EzPinNames[7] = { GPIO_0, GPIO_1, GPIO_2, GPIO_3, GPIO_4, SWCLK, SWDIO };
#else
static Gpio_t EzGpios[8] = { 0 };
static PinNames EzPinNames[8] = { GPIO_0, GPIO_1, GPIO_2, GPIO_3, GPIO_4, GPIO_5, SWCLK, SWDIO };
#endif

static I2c_t* pDeviceI2c;
static Spi_t* pDeviceSpi;
/**
 *  Calculate free SRAM
 */
int getFreeRam(void)
{
    extern char __bss_end__;

    int freeMemory;

    freeMemory = ((int)&freeMemory) - ((int)&__bss_end__);
    return freeMemory;
}

void PeriphInit(void)
{
	pDeviceI2c = DeviceGetI2c();
	pDeviceSpi = DeviceGetSpi();
}

void SetUartBaudrate( uint32_t baudrate )
{
	DeviceSetBaudeRate(baudrate);
}

uint8_t WriteI2c( uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
	deviceAddr = deviceAddr << 1;
	return I2cWriteBuffer( pDeviceI2c, deviceAddr, addr, buffer, size );
}


uint8_t ReadI2c( uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
	deviceAddr = deviceAddr << 1;
	return I2cReadBuffer( pDeviceI2c, deviceAddr, addr, buffer, size );
}

uint16_t ReadWriteSpi( uint16_t outData )
{
	return SpiInOut( pDeviceSpi, outData );
}

void InitGpio( PortNo_t no, PinModes mode, PinConfigs config, PinTypes type )
{
	GpioInit( &EzGpios[no], EzPinNames[no], mode, config, type, 0 );
}

void WriteGpio( PortNo_t no, uint32_t value )
{
		GpioWrite( &EzGpios[no], value );
}

void ToggleGpio( PortNo_t no )
{
		GpioToggle( &EzGpios[no] );
}

uint32_t ReadGpio( PortNo_t no )
{
	return GpioRead( &EzGpios[no] );
}

uint16_t ReadAdcVoltage( AdcCh_t ch )
{
	uint32_t channel = ch == CH0 ? ADC_CHANNEL_2 : ADC_CHANNEL_3;
	uint16_t millVolt = DeviceAdcMeasureVoltage(channel);
	return (uint16_t)millVolt;
}

uint16_t ReadAdc( AdcCh_t ch )
{
	uint32_t channel = ch == CH0 ? ADC_CHANNEL_2 : ADC_CHANNEL_3;
	uint16_t millVolt = DeviceAdcMeasureValue(channel);
	return (uint16_t)millVolt;
}

uint16_t ReadVdd( void )
{
	return DeviceMeasureVdd();
}

int16_t GetRssi( void )
{
	return LoRaLinkGetRssi();
}

int8_t GetSnr( void )
{
	return LoRaLinkGetSnr();
}
