/*!
 * \file      TaskMgr.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "TaskMgmt.h"
#include "Peripheral.h"
#include "utilities.h"
#include "MQTTSNDefines.h"
#include "MQTTSNClient.h"
#include "MQTTSNTopic.h"
#include "device.h"
#include "device-config.h"
#include "eeprom.h"
#include "fifo.h"
#include "gpio.h"
#include "systime.h"
#include "uart.h"
#include "LoRaLink.h"

const char theVersion[] = "0.0.0";

#define TASK_EXECUTION_TIME_UNIT     60       // 1 munit = 60 secs
#define TASK_EXECUTION_PENDING       0xff000000

#define TASK_LONG_INTERVAL           4       // Sec

#define UTC_DIFF                     9       // UTC difference

extern void LoRaLinkInitilize(void);

//static void Task_changeInterval(uint8_t id, uint16_t interval);

/*
 * Task List element
 */
typedef struct Task_s
{
	uint8_t id;
	uint8_t isRunning;
	uint32_t exTime;
	uint32_t interval;
	void    (*callback)();
	struct Task_s* next;
}Task_t;


/*!
 * Timer to handle the execution
 */
static TimerEvent_t TaskExecutionTimer;
static TimerEvent_t WakeupTimer;

/*
 *  Task Control Flags
 */
static uint8_t Task_ExecFlg = 0;
static uint8_t Task_Int0Cnt = 0;
static uint8_t Task_Int1Cnt = 0;
static uint8_t Wakeup_Flg   = 0;

static SysTime_t SystInt1 = {0};
static uint8_t LongInterval1 = 0;

LoRaLinkPacket_t*  PktReceived = NULL;

/*
 *  Task List
 */
__attribute__((weak)) TASK_LIST  = { END_OF_TASK_LIST };

/*
 *  Subscribe List
 */
__attribute__((weak)) SUBSCRIBE_LIST  = { END_OF_SUBSCRIBE_LIST };


/*
 * Print out Version
 */
void printVersion(void)
{
	printf("\r\n\r\n\r\n   %s\r\n\r\n", theVersion );
}

/*
 *  Forward declaration
 */
//static void TaskExecSubscribeList(const char* topic, Payload_t* payload);

/**
 *  Framework's functions
 */
__attribute__((weak)) void start(void)
{
}

__attribute__((weak)) void sleep(void)
{
}

__attribute__((weak)) void wakeup(void)
{
}

__attribute__((weak)) void int0(void)
{
	printf("\r\nINT0 !!!\r\n");
}

__attribute__((weak)) void int1(void)
{
	printf("\r\nINT1 !!!\r\n");
}

static void OnTaskExecutionTimerEvent(void* context)
{
	Task_ExecFlg = 1;
}

static void OnInterrupt0(void* context)
{
	Task_Int0Cnt++;
}

/*
static void OnInterrupt1(void* context)
{
	Task_Int1Cnt++;
}
*/
static void OnInterrupt1(void* context)
{
	if ( SystInt1.Seconds == 0 )
	{
		SystInt1 = SysTimeGet();
		LongInterval1 = false;
	}
	else
	{
		if ( SysTimeGet().Seconds - SystInt1.Seconds >= TASK_LONG_INTERVAL )
		{
			LongInterval1 = true;
		}
		SystInt1.Seconds = 0;
		Task_Int1Cnt++;
	}
}

static void OnWakeupTimerEvent(void* context)
{
	Wakeup_Flg = 1;
}


/*
 * Task List
 */
static Task_t* TaskListHead = 0;


static void Task_add(Task_t* task)
{
	if ( task->isRunning == 1 )
	{
		return;
	}

	task->next = 0;
	if ( TaskListHead == 0 )
	{
	    TaskListHead = task;
	}
	else
	{
	    Task_t* prev = 0;
	    Task_t* cur = TaskListHead;
	    int i = 0;
	    while(1)
	    {
	    	if ( cur == 0 )
	    	{
	    		prev->next = task;
	    		break;
	    	}
	    	else if ( cur->exTime > task->exTime )
	    	{
	    		task->next = cur;
	    		if ( i == 0 )
	    		{
	    			TaskListHead = task;
	    		}
	    		else
	    		{
	    			prev->next = task;
	    		}
	    		break;
	    	}
	    	prev = cur;
	    	cur = cur->next;
	    	i++;
	    }
	}
}


static Task_t* Task_eject(uint8_t id)
{
	Task_t* cur = TaskListHead;
	Task_t* prev = cur;

	while( cur > 0 )
	{
		if ( cur->id == id )
		{
			if ( cur->isRunning == 1 )
			{
				return cur;
			}

			if ( cur->next == 0 )
			{
				if ( prev == cur )
				{
					TaskListHead = 0;
				}
				else
				{
					prev->next = 0;
				}
			}
			else
			{
				prev->next = cur->next;
			}
			return cur;;
		}
		else
		{
			cur = cur->next;
		}
	}
	return 0;
}


void Task_print(void)
{
	Task_t* cur = TaskListHead;

	while(cur > 0)
	{
		DLOG("Task ID = %d exTime=%s \n", cur->id, SysTimeGetStrLocalTime(cur->exTime) );
		cur = cur->next;
	}
	DLOG("\r\n");
}

void SetIntMode(PinNames port, IrqModes mode, PinTypes type)
{
	if ( port == INT_0 )
	{
		GpioInit( DeviceGetInt0(), INT_0, PIN_INPUT, PIN_PUSH_PULL, type, 0 );
		GpioSetInterrupt( DeviceGetInt0(), mode, IRQ_LOW_PRIORITY, OnInterrupt0 );
	}
	else if ( port == INT_1 )
	{
//		GpioInit( DeviceGetInt1(), INT_1, PIN_INPUT, PIN_PUSH_PULL, type, 0 );
//		GpioSetInterrupt( DeviceGetInt1(), mode, IRQ_LOW_PRIORITY, OnInterrupt1 );
		printf("SetIntMode INT_1 was ignored\r\n");
	}
}

bool GetStateOfInt0(void)
{
	return GpioRead(DeviceGetInt0());
}

bool GetStateOfInt1(void)
{
	return GpioRead(DeviceGetInt1());
}

static void setInterrupt(void)
{
	TimerInit( &WakeupTimer, OnWakeupTimerEvent );

	// Setup Gpio Interruption pins
	GpioSetInterrupt( DeviceGetInt0(), IRQ_FALLING_EDGE, IRQ_LOW_PRIORITY, OnInterrupt0 );
	GpioSetInterrupt( DeviceGetInt1(), IRQ_RISING_FALLING_EDGE, IRQ_LOW_PRIORITY, OnInterrupt1 );
}

static void Task_init(void)
{
	TaskListHead = 0;
	SysTime_t time = SysTimeGet();
	Task_t* task = 0;

	for (uint8_t i = 0; theTaskList[i].callback != 0; i++)
	{
		task = malloc(sizeof(Task_t));

		task->id = i;
		task->interval = theTaskList[i].interval * TASK_EXECUTION_TIME_UNIT;
		task->isRunning = 0;
		task->next = 0;
		task->callback = theTaskList[i].callback;

		if ( task->interval == 0 )
		{
			task->exTime = TASK_EXECUTION_PENDING;
		}
		else
		{
			task->exTime = time.Seconds + theTaskList[i].start * TASK_EXECUTION_TIME_UNIT;
		}
		Task_add(task);
	}

	// Initialize Task Execution & PingReq Timer
    TimerInit( &TaskExecutionTimer, OnTaskExecutionTimerEvent );
}

static void Task_sleep(uint32_t execTime)
{
	uint32_t tSh = SysTimeGet().Seconds;

	if ( Task_Int0Cnt > 0 )
	{
		return;
	}
	else if ( Task_Int1Cnt > 0)
	{
		return;
	}
	else if ( ( execTime > 0 && tSh >= execTime ) ||  Task_ExecFlg == 1 )
	{
		TimerStop( &TaskExecutionTimer );
		Task_ExecFlg = 1;
		return;
	}
	else
	{
		if ( execTime > 0 )
		{
			Task_ExecFlg = 0;
			tSh = execTime - tSh;
			TimerSetValue( &TaskExecutionTimer, tSh * 1000 );
			TimerStart( &TaskExecutionTimer );
			TaskListHead->isRunning = 1;

//			Disconnect( tSh * 1000 );
			DeviceLowPowerHandler( );

		}
		else if ( TaskListHead->isRunning == 1 )
		{
			Task_ExecFlg = 1;
		}
	}
}


void Task_changeInterval(uint8_t id, uint16_t interval)
{
	Task_t* task = Task_eject(id);

	if ( task > 0 )
	{
		task->interval = (uint32_t)interval * TASK_EXECUTION_TIME_UNIT;

		if ( interval == 0 )
		{
			task->exTime = TASK_EXECUTION_PENDING;
		}
		else
		{
			task->exTime = SysTimeGet().Seconds + 5;
		}

		Task_add(task);
		Task_print();
	}
}


static void Task_run(void)
{
	uint32_t execTime = 0;

	while ( true )
	{
		if ( TaskListHead == 0 )
		{
			execTime = SysTimeGet().Seconds + 24 * 60 * TASK_EXECUTION_TIME_UNIT;  // 24 Hours
			Task_sleep(execTime);
		}
		else if ( TaskListHead->isRunning == 0 )
		{
			execTime = TaskListHead->exTime;
			Task_sleep(execTime);
		}
		else
		{
			DeviceLowPowerHandler( );
		}

		CheckPingRequest();

		if ( Task_Int0Cnt > 0 )
		{
			Task_Int0Cnt--;
			int0();
		}
		else if ( Task_Int1Cnt > 0 )
		{
			Task_Int1Cnt--;
			int1();
		}
		else if ( Task_ExecFlg == 1 )
		{
			Task_ExecFlg = 0;
			Task_t* task = TaskListHead;

			while( task->exTime <= SysTimeGet().Seconds )
			{
				task->callback();   //  Execute a Task   ( send MQTT-SN message in it )

				 /* Check memory leak */
				DLOG_MSG_INT( " Free  RAM = ", GetFreeRam() );

				task->isRunning = 0;
				TaskListHead = task->next;
				task->next = 0;
				if ( task->interval > 0 )
				{
					task->exTime += task->interval;
				}
				else
				{
					task->exTime = TASK_EXECUTION_PENDING;
				}
				Task_add(task);
				task = TaskListHead;
				Task_print();
			}
		}
	}
}



void WaitMs(uint32_t milsecs)
{
	TimerSetValue(&WakeupTimer, milsecs);
	TimerStart(&WakeupTimer);

	while(true )
	{
		DeviceLowPowerHandler();

		if ( Task_Int0Cnt > 0 )
		{
			Task_Int0Cnt--;
			int0();
		}
		else if ( Task_Int1Cnt > 0 )
		{
			Task_Int1Cnt--;
			int1();
		}
		else if ( Wakeup_Flg == 1 )
		{
			TimerStop( &WakeupTimer );
			Wakeup_Flg = 0;
			return;
		}
		else
		{

		}
	}
}

void WaitInt(uint32_t milsecs)
{
	TimerSetValue(&WakeupTimer, milsecs);
	TimerStart(&WakeupTimer);

	while(true )
	{
		DeviceLowPowerHandler();

		if ( Task_Int0Cnt >0 )
		{
			Task_Int0Cnt--;
			int0();
			break;
		}
		else if ( Task_Int1Cnt > 0 )
		{
			Task_Int1Cnt--;
			int1();
			break;
		}
		else if ( Wakeup_Flg == 1 )
		{
			break;
		}
		else
		{

		}
	}
	TimerStop( &WakeupTimer );
	Wakeup_Flg = 0;
	return;
}

bool IsLongInterval(void)
{
	return LongInterval1;
}

/*
 *  Application evoked.
 */
int main( void )
{
	DeviceInitMcu();
	LoRaLinkInitilize();
	SysTimeSetTimeZone( UTC_DIFF );   // Time zone is JST
	setInterrupt();
	srand1( DeviceGetRandomSeed());

	start();
	Task_init();
	Task_run();
}

