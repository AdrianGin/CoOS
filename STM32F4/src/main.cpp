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

#include "stm32f4xx_hal.h"


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

extern "C" void HAL_SYSTICK_Callback(void)
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
      i += 1;
      CoOS::WaitForSignal(SIGNAL_SYSTICK);
      CoOS::Yield();
      CoOS::ClearSignal(SIGNAL_SYSTICK);
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
   SCB->CCR |= SCB_CCR_STKALIGN_Msk;
   HAL_Init();
   SystemClock_Config();

   CoOS::RoundRobin::AddThread(&Thread0);
   CoOS::RoundRobin::AddThread(&Thread1);
  // CoOS::RoundRobin::AddThread(&Thread2);
 //  CoOS::RoundRobin::AddThread(&Thread3);

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




/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
