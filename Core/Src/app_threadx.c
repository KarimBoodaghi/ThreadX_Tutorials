/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "main.h"
#include <stdarg.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define THREAD_LED_STACK_SIZE   2048
#define THREAD_LED_PRIORITY     10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
TX_EVENT_FLAGS_GROUP key_events;
TX_THREAD thread_led;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static VOID thread_led_entry(ULONG thread_input);
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;

  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
	CHAR *stack_ptr = TX_NULL;
	TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL *)memory_ptr;

	/*------------------- Event Flags -------------------*/
	if (tx_event_flags_create(&key_events, "key_events") != TX_SUCCESS)
	{
	    return TX_GROUP_ERROR;
	}

	/* Thread LED */
	if (tx_byte_allocate(byte_pool, (VOID **)&stack_ptr, THREAD_LED_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
	{
		return TX_POOL_ERROR;
	}

	if (tx_thread_create(&thread_led, "thread_led", thread_led_entry, 0,
			stack_ptr, THREAD_LED_STACK_SIZE,
			THREAD_LED_PRIORITY, THREAD_LED_PRIORITY,
			TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
	{
		return TX_THREAD_ERROR;
	}
  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN Before_Kernel_Start */

  /* USER CODE END Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN Kernel_Start_Error */

  /* USER CODE END Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */
/*------------------- Event Flags -------------------*/
static VOID thread_led_entry(ULONG thread_input)
{
	ULONG period_ms = LED_PERIOD_DEFAULT_MS;
	ULONG actual_flags;
	ULONG ticks;
	UINT status;

	(void)thread_input;

	while (1)
	{
		/* Convert milliseconds to ticks */
		ticks = (period_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;

		/* Sleep for one period, but wake up early if Key1 or Key2 is set */
		status = tx_event_flags_get(&key_events, KEY1_EVENT | KEY2_EVENT,
				TX_OR_CLEAR, &actual_flags, ticks);

		if (status == TX_NO_EVENTS)
		{
			/* Timeout: the period elapsed */
			HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
		}
		else if (status == TX_SUCCESS)
		{
			if ((actual_flags & KEY1_EVENT) && (period_ms > LED_PERIOD_MIN_MS))
			{
				period_ms -= LED_PERIOD_STEP_MS;
			}

			if ((actual_flags & KEY2_EVENT) && (period_ms < LED_PERIOD_MAX_MS))
			{
				period_ms += LED_PERIOD_STEP_MS;
			}

			printf("period = %lu ms\r\n", period_ms);
		}
	}
}
/* USER CODE END 1 */
