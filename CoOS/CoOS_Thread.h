/*
 * CoOS_Thread.h
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#ifndef SRC_COOS_COOS_THREAD_H_
#define SRC_COOS_COOS_THREAD_H_

#include "CoOS_Conf.h"

#include "CoOS_Signal.h"

#include <cstring>

namespace CoOS {

void ThreadStart();

class Thread {

public:



   static const uint32_t EXEC_RETURN_PSP = 0xFFFFFFFD;

   typedef struct {
      uint32_t r11;
      uint32_t r10;
      uint32_t r9;
      uint32_t r8;
      uint32_t r7;
      uint32_t r6;
      uint32_t r5;
      uint32_t r4;
      uint32_t r0;
      uint32_t r1;
      uint32_t r2;
      uint32_t r3;
      uint32_t r12;
      void* lr;
      void* pc;
      uint32_t xpsr;
   } Context;

   typedef void (*FnPtr)(void);



   Thread(uint32_t start, uint32_t stackSize, SignalFlags::Flags initialWait, bool isMaster = false);

   inline void WaitForSignal(SignalFlags::Flags mask)
   {
      m_waitMask = mask;
   }

   uint32_t m_psp;
   uint32_t* m_stack;
   SignalFlags m_Flags;
   SignalFlags::Flags m_waitMask;

  // const Context** const m_stackFrame; //Only valid when task is suspended.


};

} /* namespace CoOS */

#endif /* SRC_COOS_COOS_THREAD_H_ */
