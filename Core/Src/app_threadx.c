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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define BLINK_RED_STACK_SIZE    2048
//#define BLINK_RED_PRIORITY      10

//#define BLINK_GREEN_STACK_SIZE    2048
//#define BLINK_GREEN_PRIORITY      10

//#define BUTTON_STACK_SIZE    2048
//#define BUTTON_PRIORITY      10

#define KEY_STACK_SIZE       2048
#define KEY_PRIORITY         10
#define KEY_QUEUE_MSG_COUNT  8
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
//static TX_THREAD blink_red_thread;
//static TX_THREAD blink_green_thread;

//TX_SEMAPHORE button_sem;
//static TX_THREAD button_thread;

TX_QUEUE key_queue;
static TX_THREAD key_thread;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
//static VOID blink_red_thread_entry(ULONG thread_input);
//static VOID blink_green_thread_entry(ULONG thread_input);
//static VOID button_thread_entry(ULONG thread_input);
static VOID key_thread_entry(ULONG thread_input);
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


	/*------------------- Red -------------------*/
//	if (tx_byte_allocate(byte_pool, (VOID **)&stack_ptr, BLINK_RED_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
//	{
//		return TX_POOL_ERROR;
//	}
//
//	if (tx_thread_create(&blink_red_thread, "blink_red", blink_red_thread_entry, 0,
//			stack_ptr, BLINK_RED_STACK_SIZE,
//			BLINK_RED_PRIORITY, BLINK_RED_PRIORITY,
//			TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
//	{
//		return TX_THREAD_ERROR;
//	}

	/*------------------- Green -------------------*/
//	if (tx_byte_allocate(byte_pool, (VOID **)&stack_ptr, BLINK_GREEN_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
//	{
//		return TX_POOL_ERROR;
//	}
//
//	if (tx_thread_create(&blink_green_thread, "blink_green", blink_green_thread_entry, 0,
//			stack_ptr, BLINK_GREEN_STACK_SIZE,
//			BLINK_GREEN_PRIORITY, BLINK_GREEN_PRIORITY,
//			TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
//	{
//		return TX_THREAD_ERROR;
//	}

//	/*------------------- Button Semaphore -------------------*/
//	if (tx_semaphore_create(&button_sem, "button", 0) != TX_SUCCESS)
//	{
//	    return TX_SEMAPHORE_ERROR;
//	}

	/*------------------- Button -------------------*/
//	if (tx_byte_allocate(byte_pool, (VOID **)&stack_ptr, BUTTON_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
//	{
//		return TX_POOL_ERROR;
//	}
//
//	if (tx_thread_create(&button_thread, "button", button_thread_entry, 0,
//			stack_ptr, BUTTON_STACK_SIZE,
//			BUTTON_PRIORITY, BUTTON_PRIORITY,
//			TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
//	{
//		return TX_THREAD_ERROR;
//	}

	/*------------------- Key Queue -------------------*/
	VOID *queue_ptr = TX_NULL;

	/* Allocate storage for the queue messages */
	if (tx_byte_allocate(byte_pool, &queue_ptr,
	                     KEY_QUEUE_MSG_COUNT * sizeof(ULONG), TX_NO_WAIT) != TX_SUCCESS)
	{
	    return TX_POOL_ERROR;
	}

	if (tx_queue_create(&key_queue, "key_queue", TX_1_ULONG, queue_ptr,
	                    KEY_QUEUE_MSG_COUNT * sizeof(ULONG)) != TX_SUCCESS)
	{
	    return TX_QUEUE_ERROR;
	}

	/*------------------- Key -------------------*/
	if (tx_byte_allocate(byte_pool, (VOID **)&stack_ptr, KEY_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
	{
		return TX_POOL_ERROR;
	}

	if (tx_thread_create(&key_thread, "key", key_thread_entry, 0,
			stack_ptr, KEY_STACK_SIZE,
			KEY_PRIORITY, KEY_PRIORITY,
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
/*------------------- Red -------------------*/
//static VOID blink_red_thread_entry(ULONG thread_input)
//{
//	(void)thread_input;
//
//	while (1)
//	{
//		HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
//		printf("ThreadX Red tick %lu\r\n", (unsigned long)tx_time_get());
//		tx_thread_sleep(50);                     /* 50 ticks = 500 ms at 100 ticks/s */
//	}
//}

/*------------------- Green -------------------*/
//static VOID blink_green_thread_entry(ULONG thread_input)
//{
//	(void)thread_input;
//
//	while (1)
//	{
//		HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
//		printf("ThreadX Green tick %lu\r\n", (unsigned long)tx_time_get());
//		tx_thread_sleep(100);                    /* 100 ticks = 1000 ms */
//	}
//}

/*------------------- Button -------------------*/
//static VOID button_thread_entry(ULONG thread_input)
//{
//    ULONG count = 0;
//
//    (void)thread_input;
//
//    while (1)
//    {
//        /* Sleep here until the ISR gives the semaphore */
//        tx_semaphore_get(&button_sem, TX_WAIT_FOREVER);
//
//        count++;
//        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
//        printf("Button press %lu\r\n", count);
//
//        tx_thread_sleep(5);   /* Crude debounce: 50 ms */
//    }
//}

/*------------------- Key -------------------*/
static VOID key_thread_entry(ULONG thread_input)
{
    ULONG key_id;

    (void)thread_input;

    while (1)
    {
        /* Sleep until the ISR sends a key number */
        if (tx_queue_receive(&key_queue, &key_id, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            printf("Key %lu pressed\r\n", key_id);

            switch (key_id)
            {
                case 1:
                    HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
                    break;

                case 2:
                    HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
                    break;

                case 3:
                    HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
                    break;

                default:
                    break;
            }

            tx_thread_sleep(5);   /* Crude debounce: 50 ms */
        }
    }
}

/* USER CODE END 1 */
