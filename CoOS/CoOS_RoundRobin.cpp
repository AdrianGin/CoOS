/*
 * CoOS_RoundRobin.cpp
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#include "CoOS_RoundRobin.h"

#include "CoOS.h"


namespace CoOS {

uint32_t RoundRobin::count;

void RoundRobin::AddThread(Thread* add)
{
   pool[RoundRobin::count++] = add;
}


void RoundRobin::Init()
{
   /**
    * 1. Setup Interrupts
    * 2. Set Stack Alignment
    * 3. Setup PSP Pointer
    * 4. Set Thread Mode to PSP
    */
   NVIC_SetPriority(PendSV_IRQn, 0xFF);
   NVIC_EnableIRQ(PendSV_IRQn);
   _currentTask = 0;
   _nextTask = 1;
}

void RoundRobin::SwitchThreads(void)
{
   ThreadID nextTask = kInvalidThread;
   ThreadID checkThread = _currentTask;
   do {

      checkThread++;

      if( checkThread >= count ) {
         checkThread = 0;
      }


      if( (pool[checkThread]->m_waitMask & pool[checkThread]->m_Flags.Get() ) == pool[checkThread]->m_waitMask ) {
         nextTask = checkThread;
      }

   } while( nextTask == kInvalidThread);

   _nextTask = nextTask;

}


} /* namespace CoOS */









