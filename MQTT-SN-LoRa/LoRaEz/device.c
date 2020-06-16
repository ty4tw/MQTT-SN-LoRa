/*!
 * \file      device.c
 *
 * \brief     Target board general functions implementation
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
#include "device.h"

#include "utilities.h"

#include "adc.h"
#include "device-config.h"
#include "gpio.h"
#include "i2c.h"
#include "lpm.h"
#include "stm32l0xx.h"
#include "rtc.h"
#include "spi.h"
#include "sx1276-device.h"
#include "timer.h"
#include "uart.h"

/*!
 * Vref values definition
 */
#define PDDADC_VREF_BANDGAP                             1224 // VREFINT out 1.224[V]
#define PDDADC_MAX_VALUE                                4095
#define VREFINT_CAL_ADDR                            ((uint16_t*)(uint32_t)0x1FF80078)

/*!
 * Battery level ratio (battery dependent)
 */
#define BATTERY_STEP_LEVEL                          0.25

/*!
 * Unique Devices IDs register set ( STM32L0xxx )
 */
#define         ID1                                 ( 0x1FF80050 )
#define         ID2                                 ( 0x1FF80054 )
#define         ID3                                 ( 0x1FF80064 )

/*
 * MCU objects
 */
Uart_t Uart = { 0 };

static uint32_t BaudRate = UART_BAUDRATE;

static Gpio_t DeviceInt0 = { 0 };
static Gpio_t DeviceInt1 = { 0 };
static I2c_t  DeviceI2c = { 0 };
static Spi_t  DeviceSpi = { 0 };
static Adc_t  DeviceAdc = { 0 };

static uint16_t DeviceVdda = 0;
static uint32_t DeviceVddCalib = 3000L;

/*
 *  virtual functions of an Application
 */
__weak void sleep(void)
{
}

__weak void wakeup(void)
{
}

/*!
 * Initializes the unused GPIO to a know status
 */
static void DeviceUnusedIoInit( void );

/*!
 * System Clock Configuration
 */
static void SystemClockConfig( void );

/*!
 * Used to measure and calibrate the system wake-up time from STOP mode
 */
static void CalibrateSystemWakeupTime( void );

/*!
 * System Clock Re-Configuration when waking up from STOP mode
 */
static void SystemClockReConfig( void );

/*!
 * Timer used at first boot to calibrate the SystemWakeupTime
 */
static TimerEvent_t CalibrateSystemWakeupTimeTimer;

/*!
 * Flag to indicate if the MCU is Initialized
 */
static bool McuInitialized = false;

/*!
 * Flag used to indicate if board is powered from the USB
 */
static bool UsbIsConnected = false;


I2c_t* DeviceGetI2c(void)
{
	return &DeviceI2c;
}

Spi_t* DeviceGetSpi(void)
{
	return &DeviceSpi;
}

Adc_t* DeviceGetAdc(void)
{
	return &DeviceAdc;
}

Gpio_t* DeviceGetInt0(void)
{
	return &DeviceInt0;
}

Gpio_t* DeviceGetInt1(void)
{
	return &DeviceInt1;
}

void DeviceSetBaudeRate(uint32_t baudrate)
{
	BaudRate = baudrate;
}


/*!
 * UART2 FIFO buffers size
 */
#define UART_FIFO_TX_SIZE                                2048
#define UART_FIFO_RX_SIZE                                1024

static uint8_t UartTxBuffer[UART_FIFO_TX_SIZE];
static uint8_t UartRxBuffer[UART_FIFO_RX_SIZE];

void DeviceUartInit(void)
{
	FifoInit( &Uart.FifoTx, UartTxBuffer, UART_FIFO_TX_SIZE );
	FifoInit( &Uart.FifoRx, UartRxBuffer, UART_FIFO_RX_SIZE );
	UartInit( &Uart, UART_1, UART1_TX, UART1_RX );
	UartConfig( &Uart, RX_TX, BaudRate, UART_8_BIT, UART_1_STOP_BIT, NO_PARITY, NO_FLOW_CTRL	);
}

bool DeviceIsUartsBusy(void)
{
	return ( IsFifoEmpty(&Uart.FifoTx) == false );
}

uint8_t DeviceGetChar(uint16_t mses)
{
	uint8_t data = 0;
	uint8_t prompt = '>';

	UartMcuPutChar( &Uart, prompt );

	uint64_t refTicks = RtcGetTimerValue( );
	uint64_t delayTicks = RtcMs2Tick( mses );

	do {
		if (UartMcuGetChar( &Uart, &data ) == 0 )
		{
			return data;
		}
	}while ( ( ( RtcGetTimerValue( ) - refTicks ) ) < delayTicks );

	return data;
}


/*!
 * Flag to indicate if the SystemWakeupTime is Calibrated
 */
static volatile bool SystemWakeupTimeCalibrated = false;

/*!
 * Callback indicating the end of the system wake-up time calibration
 */
static void OnCalibrateSystemWakeupTimeTimerEvent( void* context )
{
    RtcSetMcuWakeUpTime( );
    SystemWakeupTimeCalibrated = true;
}

void DeviceCriticalSectionBegin( uint32_t *mask )
{
    *mask = __get_PRIMASK( );
    __disable_irq( );
}

void DeviceCriticalSectionEnd( uint32_t *mask )
{
    __set_PRIMASK( *mask );
}

void DeviceInitPeriph( void )
{

}

void DeviceInitMcu( void )
{
    if( McuInitialized == false )
    {
        HAL_Init( );

        SystemClockConfig( );

        GpioInit( &DeviceInt0, INT_0, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
        GpioInit( &DeviceInt1, INT_1, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );

        DeviceUnusedIoInit( );
        RtcInit( );
        AdcInit( &DeviceAdc, NC );
        DeviceGetBatteryLevel();

        if( GetDevicePowerSource( ) == BATTERY_POWER )
        {
            // Disables OFF mode - Enables lowest power mode (STOP)
            LpmSetOffMode( LPM_APPLI_ID, LPM_DISABLE );
        }
    }
    else
    {
        SystemClockReConfig( );
    }

    DeviceUartInit();

	I2cInit( &DeviceI2c, I2C_1, I2C_SCL, I2C_SDA );
	SpiInit( &DeviceSpi, SPI_2, SPI2_MOSI, SPI2_MISO, SPI2_SCLK, SPI2_NSS );

    SpiInit( &SX1276.Spi, SPI_1, RADIO_MOSI, RADIO_MISO, RADIO_SCLK, NC );
    SX1276IoInit( );

    if( McuInitialized == false )
    {
        McuInitialized = true;
        SX1276IoDbgInit( );
        SX1276IoTcxoInit( );
        if( GetDevicePowerSource( ) == BATTERY_POWER )
        {
            CalibrateSystemWakeupTime( );
        }
    }
}

void DeviceResetMcu( void )
{
    CRITICAL_SECTION_BEGIN( );

    //Restart system
    NVIC_SystemReset( );
}

void DeviceDeInitMcu( void )
{
    SpiDeInit( &SX1276.Spi );
    SpiDeInit( &DeviceSpi );
    I2cDeInit( &DeviceI2c );
    SX1276IoDeInit( );
    UartDeInit( &Uart );
}

uint32_t DeviceGetRandomSeed( void )
{
    return ( ( *( uint32_t* )ID1 ) ^ ( *( uint32_t* )ID2 ) ^ ( *( uint32_t* )ID3 ) );
}

void BoardGetUniqueId( uint8_t *id )
{
    id[7] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 24;
    id[6] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 16;
    id[5] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 8;
    id[4] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) );
    id[3] = ( ( *( uint32_t* )ID2 ) ) >> 24;
    id[2] = ( ( *( uint32_t* )ID2 ) ) >> 16;
    id[1] = ( ( *( uint32_t* )ID2 ) ) >> 8;
    id[0] = ( ( *( uint32_t* )ID2 ) );
}

uint16_t DeviceMeasureVdd( void )
{
    uint16_t adcRaw = AdcReadChannel( &DeviceAdc , ADC_CHANNEL_VREFINT );
    uint32_t milliVolt = ( DeviceVddCalib * (uint32_t)(*VREFINT_CAL_ADDR) ) / ( uint32_t ) adcRaw;
    DeviceVdda = ( uint16_t ) milliVolt;
    return DeviceVdda;
}

uint16_t DeviceAdcMeasureVoltage(uint32_t channel)
{
	DeviceMeasureVdd();
    uint16_t adcRaw = AdcReadChannel( &DeviceAdc , channel );
    uint32_t mVolt = ((uint32_t)DeviceVdda * (uint32_t)adcRaw ) / (uint32_t)PDDADC_MAX_VALUE;
	return (uint16_t)mVolt;
}

uint16_t DeviceAdcMeasureValue(uint32_t channel)
{
	return AdcReadChannel( &DeviceAdc , channel );;
}

uint8_t DeviceGetBatteryLevel( void )
{
    uint8_t batteryLevel = 0;
    uint16_t measuredLevel = 0;

    measuredLevel = DeviceMeasureVdd( );

    if( measuredLevel >= 3000 )
    {
        batteryLevel = 254;
    }
    else if( measuredLevel <= 2000 )
    {
        batteryLevel = 1;
    }
    else
    {
        batteryLevel = ( measuredLevel - 2000 ) * BATTERY_STEP_LEVEL;
    }
    return batteryLevel;
}

static void DeviceUnusedIoInit( void )
{
#if defined( USE_DEBUGGER )
    HAL_DBGMCU_EnableDBGSleepMode( );
    HAL_DBGMCU_EnableDBGStopMode( );
    HAL_DBGMCU_EnableDBGStandbyMode( );
#else
    HAL_DBGMCU_DisableDBGSleepMode( );
    HAL_DBGMCU_DisableDBGStopMode( );
    HAL_DBGMCU_DisableDBGStandbyMode( );
#endif
}

void SystemClockConfig( void )
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    __HAL_RCC_PWR_CLK_ENABLE( );

    __HAL_PWR_VOLTAGESCALING_CONFIG( PWR_REGULATOR_VOLTAGE_SCALE1 );

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLLMUL_6;
    RCC_OscInitStruct.PLL.PLLDIV          = RCC_PLLDIV_3;
    if( HAL_RCC_OscConfig( &RCC_OscInitStruct ) != HAL_OK )
    {
        assert_param( FAIL );
    }

    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if( HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_1 ) != HAL_OK )
    {
        assert_param( FAIL );
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if( HAL_RCCEx_PeriphCLKConfig( &PeriphClkInit ) != HAL_OK )
    {
        assert_param( FAIL );
    }

    HAL_SYSTICK_Config( HAL_RCC_GetHCLKFreq( ) / 1000 );

    HAL_SYSTICK_CLKSourceConfig( SYSTICK_CLKSOURCE_HCLK );

    // SysTick_IRQn interrupt configuration
    HAL_NVIC_SetPriority( SysTick_IRQn, 0, 0 );
}

void CalibrateSystemWakeupTime( void )
{
    if( SystemWakeupTimeCalibrated == false )
    {
        TimerInit( &CalibrateSystemWakeupTimeTimer, OnCalibrateSystemWakeupTimeTimerEvent );
        TimerSetValue( &CalibrateSystemWakeupTimeTimer, 1000 );
        TimerStart( &CalibrateSystemWakeupTimeTimer );
        while( SystemWakeupTimeCalibrated == false )
        {

        }
    }
}

void SystemClockReConfig( void )
{
    __HAL_RCC_PWR_CLK_ENABLE( );
    __HAL_PWR_VOLTAGESCALING_CONFIG( PWR_REGULATOR_VOLTAGE_SCALE1 );

    // Enable HSI
    __HAL_RCC_HSI_CONFIG( RCC_HSI_ON );

    // Wait till HSI is ready
    while( __HAL_RCC_GET_FLAG( RCC_FLAG_HSIRDY ) == RESET )
    {
    }

    // Enable PLL
    __HAL_RCC_PLL_ENABLE( );

    // Wait till PLL is ready
    while( __HAL_RCC_GET_FLAG( RCC_FLAG_PLLRDY ) == RESET )
    {
    }

    // Select PLL as system clock source
    __HAL_RCC_SYSCLK_CONFIG ( RCC_SYSCLKSOURCE_PLLCLK );

    // Wait till PLL is used as system clock source
    while( __HAL_RCC_GET_SYSCLK_SOURCE( ) != RCC_SYSCLKSOURCE_STATUS_PLLCLK )
    {
    }
}

void SysTick_Handler( void )
{
    HAL_IncTick( );
    HAL_SYSTICK_IRQHandler( );
}

uint8_t GetDevicePowerSource( void )
{
    if( UsbIsConnected == false )
    {
        return BATTERY_POWER;
    }
    else
    {
        return USB_POWER;
    }
}

/**
  * \brief Enters Low Power Stop Mode
  *
  * \note ARM exists the function when waking up
  */
void LpmEnterStopMode( void)
{
    CRITICAL_SECTION_BEGIN( );

    DeviceDeInitMcu( );

    HAL_SuspendTick();

    // Disable the Power Voltage Detector
    HAL_PWR_DisablePVD( );

    // Clear wake up flag
    SET_BIT( PWR->CR, PWR_CR_CWUF );

    // Enable Ultra low power mode
    HAL_PWREx_EnableUltraLowPower( );

    // Enable the fast wake up from Ultra low power mode
    HAL_PWREx_EnableFastWakeUp( );

    CRITICAL_SECTION_END( );

    // Enter Stop Mode
    HAL_PWR_EnterSTOPMode( PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI );
}

/*!
 * \brief Exists Low Power Stop Mode
 */
void LpmExitStopMode( void )
{
    // Disable IRQ while the MCU is not running on HSI
    CRITICAL_SECTION_BEGIN( );

    // Initilizes the peripherals
    DeviceInitMcu( );

    HAL_ResumeTick();

    CRITICAL_SECTION_END( );
    wakeup();
}

/*!
 * \brief Enters Low Power Sleep Mode
 *
 * \note ARM exits the function when waking up
 */
void LpmEnterSleepMode( void)
{
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

void DeviceLowPowerHandler( void )
{
	sleep();
	while ( DeviceIsUartsBusy() )
	{
	}
    __disable_irq( );
    /*!
     * If an interrupt has occurred after __disable_irq( ), it is kept pending 
     * and cortex will not enter low power anyway
     */

    LpmEnterLowPower( );

    __enable_irq( );
}

/*
 * Function to be used by stdout for printf etc
 */
int _write( int fd, const void *buf, size_t count )
{
    while( UartMcuPutBuffer( &Uart, ( uint8_t* )buf, ( uint16_t )count ) != 0 ){ };
    return count;
}

/*
 * Function to be used by stdin for scanf etc
 */
int _read( int fd, const void *buf, size_t count )
{
    size_t bytesRead = 0;
    while( UartMcuGetBuffer( &Uart, ( uint8_t* )buf, count, ( uint16_t* )&bytesRead ) != 0 ){ };
    // Echo back the character
    while( UartMcuPutBuffer( &Uart, ( uint8_t* )buf, ( uint16_t )bytesRead ) != 0 ){ };
    return bytesRead;
}


#ifdef USE_FULL_ASSERT
/*
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 */
void assert_failed( uint8_t* file, uint32_t line )
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %lu\r\n", file, line) */

    printf( "Wrong parameters value: file %s on line %lu\r\n", ( const char* )file, line );
    /* Infinite loop */
    while( 1 )
    {
    }
}
#endif
