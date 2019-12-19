/*
 * main.cpp
 *
 *  Created on: 10/10/2019
 *      Author: AdrianDesk
 */


#include "CoOS/Tests/Tests.h"

#include "CoOS/CoOS_Thread.h"
#include "CoOS/CoOS_RoundRobin.h"
#include "CoOS/CoOS.h"


enum {
   SIGNAL_SYSTICK = 0x01,
};

void Task0(void);
void Task1(void);
void Task2(void);
void Task3(void);

CoOS::Thread Thread0( (uint32_t)&Task0, 256, 0, true);
CoOS::Thread Thread1( (uint32_t)&Task1, 256, 0);
CoOS::Thread Thread2( (uint32_t)&Task2, 256, 0);
CoOS::Thread Thread3( (uint32_t)&Task3, 256, 0);


void SystemClock_Config(void);

void HAL_SYSTICK_Callback(void)
{
   CoOS::ISR_SignalTest();
   Thread1.m_Flags.Set(SIGNAL_SYSTICK);
}


void Task0(void)
{
   volatile static uint32_t i = 0;
   {
      i++;
      CoOS::Yield();
   }
}

void Task1(void)
{
   volatile uint32_t i = 0;
   while(1)
   {
      i += 10;
      //CoOS::WaitForSignal(SIGNAL_SYSTICK);
      CoOS::Yield();
      //CoOS::ClearSignal(SIGNAL_SYSTICK);
   }
}

void Task2(void)
{
   volatile static uint32_t i = 0;
   while(1)
   {
      i += 2;
      CoOS::Yield();
   }
}


void Task3(void)
{
   volatile static uint32_t i = 0;
   while(1)
   {
      i += 3;
      CoOS::Yield();
   }
}



int main(void)
{
   CoOS::RoundRobin::AddThread(&Thread0);
   CoOS::RoundRobin::AddThread(&Thread1);
   CoOS::RoundRobin::AddThread(&Thread2);
   CoOS::RoundRobin::AddThread(&Thread3);

   CoOS::RoundRobin::Init();
   CoOS_InitProcessStack();

   while(1)
   {
      Task0();
      //CoOS::Tests_Signal();
   }

}



/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}


