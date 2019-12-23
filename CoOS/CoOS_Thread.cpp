/*
 * CoOS_Thread.cpp
 *
 *  Created on: 12/10/2019
 *      Author: AdrianDesk
 */

#include <CoOS/CoOS_Thread.h>

#include <new>

namespace CoOS {

void ThreadStart()
{
	while(1)
	{

	}
}

const uint32_t initVal = 0xA5A50000;

Thread::Thread(uint32_t start, uint32_t stackSize, SignalFlags::Flags initialWait, uint8_t taskId)
: m_waitMask(initialWait) //, m_stackFrame( (const Context**)&m_psp )
{
   m_stack = (uint32_t*)new (std::nothrow) uint8_t[stackSize];

   memset(m_stack, 0, stackSize);

   if( taskId == 0 ) {
      //Top of stack, no context
      m_psp = (uint32_t)((uint8_t*)m_stack + stackSize);
      Context* context = (Context*)(m_psp - sizeof(Context)) ;
      context->lr = (void*)0xFFFFFFF9;

      memset(&context->r11, initVal, 13*4 );

   } else {
      //Initialised empty context
      Context* context = (Context*)(( (uint8_t*)m_stack + stackSize) - sizeof(Context)) ;

      memset(&context->r11, initVal, 13*4 );
      uint32_t id = 0;
      context->r0 = initVal + (taskId << 4) + id++;
      context->r1 = initVal + (taskId << 4) + id++;
      context->r2 = initVal + (taskId << 4) + id++;
      context->r3 = initVal + (taskId << 4) + id++;
      context->r4 = initVal + (taskId << 4) + id++;
      context->r5 = initVal + (taskId << 4) + id++;
      context->r6 = initVal + (taskId << 4) + id++;
      context->r7 = initVal + (taskId << 4) + id++;
      context->r8 = initVal + (taskId << 4) + id++;
      context->r9 = initVal + (taskId << 4) + id++;
      context->r10 = initVal + (taskId << 4) + id++;
      context->r11 = initVal + (taskId << 4) + id++;
      context->r12 = initVal + (taskId << 4) + id++;

      context->pc = (void*)start;
      context->lr = (void*)(&ThreadStart + 2); //+2 is required for the GDB debugger to place it FULLY into the function
      context->xpsr = 0x01000000; // initial xPSR
      m_psp = (uint32_t)m_stack + stackSize - sizeof(Context);
      context->r7 = (uint32_t)m_stack + stackSize;

   }
}

} /* namespace CoOS */
