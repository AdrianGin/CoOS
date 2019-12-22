/*
 * CoOS_Thread.cpp
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#include <CoOS/CoOS_Thread.h>

#include <new>

namespace CoOS {

static void ThreadStart()
{
	while(1)
	{

	}
}

Thread::Thread(uint32_t start, uint32_t stackSize, SignalFlags::Flags initialWait, bool isMaster) : m_waitMask(initialWait)   {
   m_stack = (uint32_t*)new (std::nothrow) uint8_t[stackSize];

   memset(m_stack, 0, stackSize);

   if( isMaster ) {
      //Top of stack, no context
      m_psp = (uint32_t)((uint8_t*)m_stack + stackSize);
      Context* context = (Context*)(( (uint8_t*)m_stack + stackSize) - sizeof(Context)) ;
      context->lr = 0xFFFFFFF9;


   } else {
      //Initialised empty context
      Context* context = (Context*)(( (uint8_t*)m_stack + stackSize) - sizeof(Context)) ;
      context->pc = (uint32_t)start;
      context->lr = (uint32_t)&ThreadStart + 2; //+2 is required for the GDB debugger to place it FULLY into the function
      context->xpsr = 0x01000000; // initial xPSR
      m_psp = (uint32_t)m_stack + stackSize - sizeof(Context);
   }
}

} /* namespace CoOS */
