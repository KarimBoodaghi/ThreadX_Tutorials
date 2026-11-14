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

AppState_t AppState = { BLINK_MODE_R, LED_PERIOD_DEFAULT_MS, 0 };

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
    AppState_t snapshot;
    ULONG actual_flags;
    ULONG wait_ticks;
    UINT status;
    UINT color_turn = 0;
    ULONG last_mode = BLINK_MODE_R;

    (void)thread_input;

    while (1)
    {
        /* Take a stable copy of the shared state */
        tx_mutex_get(&state_mutex, TX_WAIT_FOREVER);
        snapshot = AppState;
        tx_mutex_put(&state_mutex);

        if (snapshot.mode != last_mode)
        {
            /* Turn all LEDs off here (active low), then remember the mode */
        	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);

            last_mode = snapshot.mode;
        }

        /* Compute wait_ticks from snapshot.period_ms and snapshot.paused */
        if (snapshot.paused)
        {
            wait_ticks = TX_WAIT_FOREVER;
        }
        else
        {
            wait_ticks = (snapshot.period_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;
        }

        status = tx_event_flags_get(&key_events, LED_EVENT, TX_OR_CLEAR,
                                    &actual_flags, wait_ticks);

        if (status == TX_NO_EVENTS)
        {
			/* Timeout: the period elapsed */
			if(snapshot.mode == BLINK_MODE_R)
				HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
			else if(snapshot.mode == BLINK_MODE_G)
				HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
			else if(snapshot.mode == BLINK_MODE_B)
				HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
			else if(snapshot.mode == BLINK_MODE_CIRCULAR)
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
	UINT changed;

	(void)thread_input;

	while (1)
	{
		if (tx_queue_receive(&key_queue, &msg, TX_WAIT_FOREVER) == TX_SUCCESS)
		{
			changed = 0;

			tx_mutex_get(&state_mutex, TX_WAIT_FOREVER);

			switch (msg.key_id)
			{
			case 1:
				/* Faster: decrease the period if it is above the minimum */
				if (AppState.period_ms > LED_PERIOD_MIN_MS)
				{
					AppState.period_ms -= LED_PERIOD_STEP_MS;
					changed = 1;
				}
				break;

			case 2:
			    /* Slower: increase the period if it is below the maximum */
			    if (AppState.period_ms < LED_PERIOD_MAX_MS)
			    {
			        AppState.period_ms += LED_PERIOD_STEP_MS;
			        changed = 1;
			    }
			    break;

			case 3:
				/* Pause or resume */
				AppState.paused = !AppState.paused;
				changed = 1;
				break;

			case 4:
				/* Next mode, then wrap after the last one */
				AppState.mode = AppState.mode + 1;
	        	if(AppState.mode > BLINK_MODE_CIRCULAR)
	        		AppState.mode = BLINK_MODE_R;
	        	changed = 1;
				break;

			default:
				break;
			}

			tx_mutex_put(&state_mutex);

			if (changed)
			{
				tx_event_flags_set(&key_events, LED_EVENT | CHANGE_EVENT, TX_OR);
			}
		}
	}
}
/* USER CODE END 1 */
