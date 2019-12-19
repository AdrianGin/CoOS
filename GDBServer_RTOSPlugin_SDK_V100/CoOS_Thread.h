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

class Thread {

public:



   static const uint32_t EXEC_RETURN_PSP = 0xFFFFFFFD;

   typedef struct {
      uint32_t rx[11-4+1];
      uint32_t r[5];
      uint32_t lr;
      uint32_t pc;
      uint32_t xpsr;
   } Context;

   typedef void (*FnPtr)(void);



   Thread(FnPtr start, uint32_t stackSize, SignalFlags::Flags initialWait, bool isMaster = false);

   inline void WaitForSignal(SignalFlags::Flags mask)
   {
      m_waitMask = mask;
   }

   uint32_t m_psp;
   uint32_t* m_stack;
   SignalFlags m_Flags;
   SignalFlags::Flags m_waitMask;

};

} /* namespace CoOS */

#endif /* SRC_COOS_COOS_THREAD_H_ */
