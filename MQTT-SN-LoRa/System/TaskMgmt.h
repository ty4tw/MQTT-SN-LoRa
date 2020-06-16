/*!
 * \file      TaskMgr.h
 *
 * \copyright Revised BSD License, see section \ref LICENSE
 * (C)2019 Tomy-Technology
 * \author  Tomoaki Yamaguch
 */
#ifndef LORA_APPCONTROL_H_
#define LORA_APPCONTROL_H_

#include "Payload.h"

#include "gpio.h"
#include "timer.h"

/*
 * Type definitions
 */
typedef struct
{
    void (*callback)(void);
    uint32_t start;
    uint32_t interval;
} TaskList_t;

/*
typedef struct PortList
{
    uint8_t port;
    void (*callback)(Payload_t* payload);
} PortList_t;
*/

typedef struct
{
	const char* topic;
	int (*pubCallback)(Payload_t*);
	uint8_t qos;
} OnPublishList_t;

#define TASK_LIST   TaskList_t  theTaskList[]
#define TASK(...)         {__VA_ARGS__}
#define END_OF_TASK_LIST  {0, 0, 0}

#ifndef SUBSCRIBE_LIST
#define SUBSCRIBE_LIST    OnPublishList_t theOnPublishList[]
#define SUB(...)          {__VA_ARGS__}
#define END_OF_SUBSCRIBE_LIST {0,0,0}
#endif

void WaitMs(uint32_t milsecs);
void WaitInt(uint32_t milsecs);

bool IsLongInterval(void);
bool GetStateOfInt0(void);
bool GetStateOfInt1(void);

/*!
 * \brief Set Interrupt0 condition
 *
 * \param [IN] port       INT_0 /  INT_1
 * \param [IN] mode       NO_IRQ        / IRQ_RISING_EDGE /  IRQ_FALLING_EDGE /  IRQ_RISING_FALLING_EDGE
 * \param [IN] type       PIN_NO_PULL   / PIN_PULL_UP     /  PIN_PULL_DOWN
 */
void SetIntMode(PinNames port, IrqModes mode, PinTypes type);



#endif /* LORA_APPCONTROL_H_ */
