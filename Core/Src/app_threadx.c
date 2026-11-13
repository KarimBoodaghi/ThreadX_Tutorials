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

#define THREAD_REPORT_STACK_SIZE   2048
#define THREAD_REPORT_PRIORITY     11 /* Lower priority */

#define REPORT_PERIOD_MS    2000

#define KEY_QUEUE_MSG_COUNT    8
#define THREAD_KEY_STACK_SIZE   2048
#define THREAD_KEY_PRIORITY     5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
TX_EVENT_FLAGS_GROUP key_events;
TX_THREAD thread_led;
TX_THREAD thread_report;

TX_MUTEX print_mutex; /* Protect printf */
TX_MUTEX state_mutex; /* Protect AppState */

typedef struct
{
    ULONG mode;         /* Current LED mode */
    ULONG period_ms;    /* Blink period in milliseconds */
    ULONG paused;       /* 0 = running, 1 = paused */
} AppState_t;

AppState_t AppState;

/* Queue for print number and time of the pressed key */
TX_QUEUE key_queue;
TX_THREAD thread_key;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static VOID thread_led_entry(ULONG thread_input);
static VOID thread_report_entry(ULONG thread_input);
static VOID thread_key_entry(ULONG thread_input);
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

	/*------------------- Mutex for printfs -------------------*/
	if (tx_mutex_create(&print_mutex, "print", TX_INHERIT) != TX_SUCCESS)
	{
	    return TX_MUTEX_ERROR;
	}

	/*------------------- Mutex for AppState -------------------*/
	if (tx_mutex_create(&state_mutex, "state", TX_INHERIT) != TX_SUCCESS)
	{
	    return TX_MUTEX_ERROR;
	}

	/*------------------- Thread LED -------------------*/
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

	/*------------------- Thread Report -------------------*/
	if (tx_byte_allocate(byte_pool, (VOID **)&stack_ptr, THREAD_REPORT_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
	{
		return TX_POOL_ERROR;
	}

	if (tx_thread_create(&thread_report, "thread_report", thread_report_entry, 0,
			stack_ptr, THREAD_REPORT_STACK_SIZE,
			THREAD_REPORT_PRIORITY, THREAD_REPORT_PRIORITY,
			TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
	{
		return TX_THREAD_ERROR;
	}

	/*------------------- Key Queue -------------------*/
	VOID *queue_ptr = TX_NULL;

	/* Allocate storage for the queue messages */
	if (tx_byte_allocate(byte_pool, &queue_ptr,
	                     KEY_QUEUE_MSG_COUNT * sizeof(KeyMessage_t), TX_NO_WAIT) != TX_SUCCESS)
	{
	    return TX_POOL_ERROR;
	}

	if (tx_queue_create(&key_queue, "key_queue", TX_2_ULONG, queue_ptr,
            KEY_QUEUE_MSG_COUNT * sizeof(KeyMessage_t)) != TX_SUCCESS)
	{
	    return TX_QUEUE_ERROR;
	}

	/*------------------- Thread Key -------------------*/
	if (tx_byte_allocate(byte_pool, (VOID **)&stack_ptr, THREAD_KEY_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
	{
		return TX_POOL_ERROR;
	}

	if (tx_thread_create(&thread_key, "thread_key", thread_key_entry, 0,
			stack_ptr, THREAD_KEY_STACK_SIZE,
			THREAD_KEY_PRIORITY, THREAD_KEY_PRIORITY,
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
/*------------------- safe_printf -------------------*/
void safe_printf(const char *fmt, ...)
{
	va_list args;

	tx_mutex_get(&print_mutex, TX_WAIT_FOREVER);

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	tx_mutex_put(&print_mutex);
}

/*------------------- thread_led -------------------*/
static VOID thread_led_entry(ULONG thread_input)
{
	AppState.period_ms = LED_PERIOD_DEFAULT_MS;
	ULONG actual_flags;
	ULONG ticks;
	UINT status;
	AppState.paused = 0;
	ULONG wait_ticks;
	AppState.mode = BLINK_MODE_R;
	UINT color_turn = 0;

	(void)thread_input;

	while (1)
	{
		/* Convert milliseconds to ticks */
		ticks = (AppState.period_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;

	    /* While paused there is no period to wait for, only a key press */
	    wait_ticks = AppState.paused ? TX_WAIT_FOREVER : ticks;

	    /* Wait for Key1, Key2 and Key3, so Key3 can also resume */
	    status = tx_event_flags_get(&key_events,
	                                KEY1_EVENT | KEY2_EVENT | KEY3_EVENT | KEY4_EVENT,
	                                TX_OR_CLEAR, &actual_flags, wait_ticks);

		if (status == TX_NO_EVENTS)
		{
			/* Timeout: the period elapsed */
			if(AppState.mode == BLINK_MODE_R)
				HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
			else if(AppState.mode == BLINK_MODE_G)
				HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
			else if(AppState.mode == BLINK_MODE_B)
				HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
			else if(AppState.mode == BLINK_MODE_CIRCULAR)
			{
				switch (color_turn)
				{
				case 0:
		        	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
		        	HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
		        	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
		        	color_turn = 1;
					break;
				case 1:
		        	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
		        	HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
		        	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
		        	color_turn = 2;
					break;
				case 2:
		        	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
		        	HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
		        	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
		        	color_turn = 0;
					break;
				}
			}
		}
		else if (status == TX_SUCCESS)
		{
			tx_mutex_get(&state_mutex, TX_WAIT_FOREVER);   /* Take the key */

			if ((actual_flags & KEY1_EVENT) && (AppState.period_ms > LED_PERIOD_MIN_MS))
			{
				AppState.period_ms -= LED_PERIOD_STEP_MS;
				tx_event_flags_set(&key_events, CHANGE_EVENT, TX_OR);
			}

			if ((actual_flags & KEY2_EVENT) && (AppState.period_ms < LED_PERIOD_MAX_MS))
			{
				AppState.period_ms += LED_PERIOD_STEP_MS;
				tx_event_flags_set(&key_events, CHANGE_EVENT, TX_OR);
			}

	        if (actual_flags & KEY3_EVENT)
	        {
	        	/* Pause or Continue Blinking */
	        	AppState.paused = !AppState.paused;
	        	tx_event_flags_set(&key_events, CHANGE_EVENT, TX_OR);
	        }

	        if (actual_flags & KEY4_EVENT)
	        {
	        	/* Go to next mode */
	        	AppState.mode = AppState.mode + 1;
	        	tx_event_flags_set(&key_events, CHANGE_EVENT, TX_OR);

	        	if(AppState.mode > BLINK_MODE_CIRCULAR)
	        		AppState.mode = BLINK_MODE_R;

	        	/* Clear LEDs (Active Low) */
	        	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
	        	HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
	        	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
	        }

	        tx_mutex_put(&state_mutex);                    /* Return the key */
		}
	}
}

/*------------------- thread_report -------------------*/
static VOID thread_report_entry(ULONG thread_input)
{
	AppState_t snapshot;
	ULONG actual_flags;
	UINT status;

	(void)thread_input;

	while (1)
	{
		status = tx_event_flags_get(&key_events,
				CHANGE_EVENT,
				TX_OR_CLEAR, &actual_flags,
				(REPORT_PERIOD_MS * TX_TIMER_TICKS_PER_SECOND) / 1000);

		if((status == TX_SUCCESS) || (status == TX_NO_EVENTS))
		{
			/* Copy the state while holding the lock, then release it */
			tx_mutex_get(&state_mutex, TX_WAIT_FOREVER);
			snapshot = AppState;
			tx_mutex_put(&state_mutex);

			/* Print without holding the state lock */
			safe_printf("[REPORT] mode=%lu period=%lums paused=%lu\r\n",
					snapshot.mode, snapshot.period_ms, snapshot.paused);
		}
	}
}

/*------------------- thread_key -------------------*/
static VOID thread_key_entry(ULONG thread_input)
{
    KeyMessage_t msg;

    (void)thread_input;

    while (1)
    {
        /* Sleep until the ISR sends a key msg */
        if (tx_queue_receive(&key_queue, &msg, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            const char *name;

             switch (msg.key_id)
             {
             case 1:
            	 name = "Key1";
             break;
             case 2:
            	 name = "Key2";
            	 break;
             case 3:
            	 name = "Key3";
            	 break;
             case 4:
            	 name = "Key4";
            	 break;
             default:
            	 name = "Key?";
            	 break;
             }

             safe_printf("[KEY] %s pressed at tick=%lu\r\n", name, msg.tick);
        }
    }
}
/* USER CODE END 1 */
