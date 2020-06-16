/*!
 * \file      TaskMgr.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
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



#define TASK_LIST   TaskList_t  theTaskList[]
#define TASK(...)         {__VA_ARGS__}
#define END_OF_TASK_LIST  {0, 0, 0}


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

/*
 * Print out Version
 */
void printVersion(void);

#endif /* LORA_APPCONTROL_H_ */
