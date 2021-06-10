/*!
 * \file      i2c.c
 *
 * \brief     I2C driver implementation
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
#include <stdbool.h>
#include "stm32l0xx.h"
#include "i2c.h"
#include "utilities.h"
#include "device-config.h"

#define I2CFREQ   100000
#define TIMEOUT_MAX                                 0x8000

/*!
 * Operation Mode for the I2C
 */
typedef enum
{
    MODE_I2C = 0,
    MODE_SMBUS_DEVICE,
    MODE_SMBUS_HOST
}I2cMode;

/*!
 * I2C signal duty cycle
 */
typedef enum
{
    I2C_DUTY_CYCLE_2 = 0,
    I2C_DUTY_CYCLE_16_9
}I2cDutyCycle;

/*!
 * I2C select if the acknowledge in after the 7th or 10th bit
 */
typedef enum
{
    I2C_ACK_ADD_7_BIT = 0,
    I2C_ACK_ADD_10_BIT
}I2cAckAddrMode;

/*!
 * Internal device address size
 */
typedef enum
{
    I2C_ADDR_SIZE_8 = 0,
    I2C_ADDR_SIZE_16,
}I2cAddrSize;

static I2C_HandleTypeDef I2cHandle = { 0 };

static I2cAddrSize I2cInternalAddrSize = I2C_ADDR_SIZE_8;

static uint32_t i2cFreq = 0;

/*!
 * Flag to indicates if the I2C is initialized
 */
static bool I2cInitialized = false;

void I2cFormat( I2c_t *obj, I2cMode mode, I2cDutyCycle dutyCycle, bool I2cAckEnable, I2cAckAddrMode AckAddrMode, uint32_t I2cFrequency )
{
    __HAL_RCC_I2C1_CLK_ENABLE( );
    i2cFreq = I2cFrequency;

    if( I2cFrequency == 100000 )
    {
        I2cHandle.Init.Timing = 0x00707CBB;
    }
    else if( I2cFrequency == 400000 )
    {
        I2cHandle.Init.Timing = 0x00300F38;
    }

    I2cHandle.Init.OwnAddress1 = 0;
    I2cHandle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    I2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    I2cHandle.Init.OwnAddress2 = 0;
    I2cHandle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    I2cHandle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    HAL_I2C_Init( &I2cHandle );

    HAL_I2CEx_ConfigAnalogFilter( &I2cHandle, I2C_ANALOGFILTER_ENABLE );
}


void I2cInit( I2c_t *obj, I2cId_t i2cId, PinNames scl, PinNames sda )
{
    if( I2cInitialized == false )
    {
        I2cInitialized = true;

        __HAL_RCC_I2C1_CLK_DISABLE( );
        __HAL_RCC_I2C1_CLK_ENABLE( );
        __HAL_RCC_I2C1_FORCE_RESET( );
        __HAL_RCC_I2C1_RELEASE_RESET( );

        obj->I2cId = i2cId;

        I2cHandle.Instance  = ( I2C_TypeDef * )I2C1_BASE;

        GpioInit( &obj->Scl, scl, PIN_ALTERNATE_FCT, PIN_OPEN_DRAIN, PIN_NO_PULL, GPIO_AF4_I2C1 );
        GpioInit( &obj->Sda, sda, PIN_ALTERNATE_FCT, PIN_OPEN_DRAIN, PIN_NO_PULL, GPIO_AF4_I2C1 );
        I2cFormat( obj, MODE_I2C, I2C_DUTY_CYCLE_2, true, I2C_ACK_ADD_7_BIT, I2CFREQ );
    }
}

void I2cDeInit( I2c_t *obj )
{
    I2cInitialized = false;

    HAL_I2C_DeInit( &I2cHandle );

    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
    __HAL_RCC_I2C1_CLK_DISABLE( );

    GpioInit( &obj->Scl, obj->Scl.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &obj->Sda, obj->Sda.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

void I2cResetBus( I2c_t *obj )
{
    __HAL_RCC_I2C1_CLK_DISABLE( );
    __HAL_RCC_I2C1_CLK_ENABLE( );
    __HAL_RCC_I2C1_FORCE_RESET( );
    __HAL_RCC_I2C1_RELEASE_RESET( );

    GpioInit( &obj->Scl, I2C_SCL, PIN_ALTERNATE_FCT, PIN_OPEN_DRAIN, PIN_NO_PULL, GPIO_AF4_I2C1 );
    GpioInit( &obj->Sda, I2C_SDA, PIN_ALTERNATE_FCT, PIN_OPEN_DRAIN, PIN_NO_PULL, GPIO_AF4_I2C1 );

    I2cFormat( obj, MODE_I2C, I2C_DUTY_CYCLE_2, true, I2C_ACK_ADD_7_BIT, i2cFreq );
}

void I2cSetAddrSize( I2c_t *obj, I2cAddrSize addrSize )
{
    I2cInternalAddrSize = addrSize;
}

uint8_t I2cMcuWriteBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
    uint8_t status = FAIL;
    uint16_t memAddSize = 0;

    if( I2cInternalAddrSize == I2C_ADDR_SIZE_8 )
    {
        memAddSize = I2C_MEMADD_SIZE_8BIT;
    }
    else
    {
        memAddSize = I2C_MEMADD_SIZE_16BIT;
    }
    status = ( HAL_I2C_Mem_Write( &I2cHandle, deviceAddr, addr, memAddSize, buffer, size, 2000 ) == HAL_OK ) ? SUCCESS : FAIL;

    return status;
}

uint8_t I2cMcuReadBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
    uint8_t status = FAIL;
    uint16_t memAddSize = 0;

    if( I2cInternalAddrSize == I2C_ADDR_SIZE_8 )
    {
        memAddSize = I2C_MEMADD_SIZE_8BIT;
    }
    else
    {
        memAddSize = I2C_MEMADD_SIZE_16BIT;
    }
    status = ( HAL_I2C_Mem_Read( &I2cHandle, deviceAddr, addr, memAddSize, buffer, size, 2000 ) == HAL_OK ) ? SUCCESS : FAIL;

    return status;
}

uint8_t I2cMcuWaitStandbyState( I2c_t *obj, uint8_t deviceAddr )
{
    uint8_t status = FAIL;

    status = ( HAL_I2C_IsDeviceReady( &I2cHandle, deviceAddr, 300, 4096 ) == HAL_OK ) ? SUCCESS : FAIL;;

    return status;
}

uint8_t I2cWrite( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t data )
{
    if( I2cInitialized == true )
    {
        if( I2cMcuWriteBuffer( obj, deviceAddr, addr, &data, 1 ) == FAIL )
        {
            // if first attempt fails due to an IRQ, try a second time
            if( I2cMcuWriteBuffer( obj, deviceAddr, addr, &data, 1 ) == FAIL )
            {
                return FAIL;
            }
            else
            {
                return SUCCESS;
            }
        }
        else
        {
            return SUCCESS;
        }
    }
    else
    {
        return FAIL;
    }
}

uint8_t I2cWriteBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
    if( I2cInitialized == true )
    {
        if( I2cMcuWriteBuffer( obj, deviceAddr, addr, buffer, size ) == FAIL )
        {
            // if first attempt fails due to an IRQ, try a second time
            if( I2cMcuWriteBuffer( obj, deviceAddr, addr, buffer, size ) == FAIL )
            {
                return FAIL;
            }
            else
            {
                return SUCCESS;
            }
        }
        else
        {
            return SUCCESS;
        }
    }
    else
    {
        return FAIL;
    }
}

uint8_t I2cRead( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *data )
{
    if( I2cInitialized == true )
    {
        return( I2cMcuReadBuffer( obj, deviceAddr, addr, data, 1 ) );
    }
    else
    {
        return FAIL;
    }
}

uint8_t I2cReadBuffer( I2c_t *obj, uint8_t deviceAddr, uint16_t addr, uint8_t *buffer, uint16_t size )
{
    if( I2cInitialized == true )
    {
        return( I2cMcuReadBuffer( obj, deviceAddr, addr, buffer, size ) );
    }
    else
    {
        return FAIL;
    }
}
