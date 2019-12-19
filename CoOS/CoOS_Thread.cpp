/*
 * CoOS_Thread.cpp
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#include <CoOS/CoOS_Thread.h>

namespace CoOS {


Thread::Thread(uint32_t start, uint32_t stackSize, SignalFlags::Flags initialWait, bool isMaster) : m_waitMask(initialWait)   {
   m_stack = (uint32_t*)new uint8_t[stackSize];

   memset(m_stack, 0, stackSize);

   if( isMaster ) {
      //Top of stack, no context
      m_psp = (uint32_t)((uint8_t*)m_stack + stackSize);
   } else {
      //Initialised empty context
      Context* context = (Context*)(( (uint8_t*)m_stack + stackSize) - sizeof(Context)) ;
      context->pc = (uint32_t)start;
      context->lr = EXEC_RETURN_PSP;
      context->xpsr = 0x01000000; // initial xPSR
      m_psp = (uint32_t)m_stack + stackSize - sizeof(Context);
   }
}

} /* namespace CoOS */
