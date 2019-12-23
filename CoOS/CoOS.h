/*
 * CoOS.h
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#ifndef SRC_COOS_TESTS_COOS_H_
#define SRC_COOS_TESTS_COOS_H_

#include "CoOS_Conf.h"
#include "CoOS_Thread.h"

#define svc(code) asm volatile ("svc %[immediate]"::[immediate] "I" (code))


extern "C" {

enum {
   SVC_START_OS = 0x00,
};

typedef int32_t ThreadID;

static const ThreadID kInvalidThread = -1;

extern volatile uint32_t _currentTask;
extern volatile uint32_t _nextTask;
extern uint32_t _svc_exec_return;

extern CoOS::Thread* pool[];
void CoOS_InitMainStack(void);
void CoOS_InitProcessStack(void);
void CoOS_Yield();

void CoOS_InterruptHandler();


void CoOS_ScheduleNextTask();

void SVC_Handler_C(uint32_t* svc_args);
}


namespace CoOS {

void WaitForSignal(SignalFlags::Flags signal);

void ClearSignal(SignalFlags::Flags signal);

//Applies to current thread
void Yield();

void InitProcessStack(void) __attribute((naked));

void InitMainStack(void) __attribute((naked));

void StartOS();

}



#endif /* SRC_COOS_TESTS_COOS_H_ */
