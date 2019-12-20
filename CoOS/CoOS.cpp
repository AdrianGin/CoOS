/*
 * CoOS.cpp
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */


#include "CoOS.h"
#include "CoOS_RoundRobin.h"

extern "C" {

volatile uint32_t _currentTask;
volatile uint32_t _nextTask;
volatile uint8_t  _isRunning = 0;
CoOS::Thread* pool[NUM_THREADS];


void SVC_Handler_C(uint32_t* svc_args)
{
   unsigned int svc_number;
   /* Stack contains:
    * r0, r1, r2, r3, r12, r14, the return address and xPSR
    * First argument (r0) is svc_args[0]
    */
   svc_number = ((char *)svc_args[6])[-2];

   switch( svc_number )
   {
      case SVC_START_OS:
         CoOS_InitProcessStack();
         return;

      default:
         break;
   }
}
}


void CoOS::WaitForSignal(SignalFlags::Flags signal)
{
   pool[_currentTask]->WaitForSignal(signal);
}

void CoOS::ClearSignal(SignalFlags::Flags signal)
{
   pool[_currentTask]->m_Flags.Clear(signal);
}

void CoOS::Yield(void)
{
   CoOS::RoundRobin::SwitchThreads();
   SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}





