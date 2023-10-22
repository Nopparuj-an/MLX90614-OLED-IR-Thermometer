/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <mlx90614.h>
#include <ssd1306.h>
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t mode = 0; // 0 CONTINUOUS, 1 SINGLE
uint8_t single_flag = 0;
float Ambient;
float Object;
float emissivity;

char buffer[20];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void encoder_handler();
void button_handler();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_I2C2_Init();
	MX_TIM3_Init();
	/* USER CODE BEGIN 2 */

	ssd1306_Init();
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		button_handler();

		// Take measurements
		if (!mode || single_flag) {
			single_flag = 0;
			Ambient = MLX90614_ReadTemp(MLX90614_DEFAULT_SA, MLX90614_TAMB);
			Object = MLX90614_ReadTemp(MLX90614_DEFAULT_SA, MLX90614_TOBJ1);
		}
		emissivity = MLX90614_ReadReg(MLX90614_DEFAULT_SA, MLX90614_EMISSIVITY, 0) / 65535.0;

		// Draw mode
		ssd1306_Fill(Black);
		if (!mode) {
			ssd1306_SetCursor(29, 0);
			ssd1306_WriteString("CONTINUOUS", Font_7x10, White);
		} else {
			ssd1306_SetCursor(43, 0);
			ssd1306_WriteString("SINGLE", Font_7x10, White);
		}

		// Draw Object temp
		ssd1306_SetCursor(5, 13);
		ssd1306_WriteString("TO:", Font_11x18, White);
		sprintf(buffer, "%.2f", Object);
		ssd1306_SetCursor(40 + (8 - strlen(buffer)) * 11, 13);
		ssd1306_WriteString(buffer, Font_11x18, White);

		// Draw Ambient temp
		ssd1306_SetCursor(5, 33);
		ssd1306_WriteString("TA:", Font_11x18, White);
		sprintf(buffer, "%.2f", Ambient);
		ssd1306_SetCursor(40 + (8 - strlen(buffer)) * 11, 33);
		ssd1306_WriteString(buffer, Font_11x18, White);

		// Draw Emissivity
		ssd1306_SetCursor(5, 53);
		sprintf(buffer, "Emissivity: %.2f", emissivity);
		ssd1306_WriteString(buffer, Font_7x10, White);

		// Finalize screen draw
		ssd1306_UpdateScreen();

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

void button_handler() {
	uint8_t button_now = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
	static uint32_t time_held = 0;
	static uint8_t run_once = 0;

	if (button_now) {
		// button up
		time_held = HAL_GetTick();
		run_once = 1;
	} else {
		// button down
		if (HAL_GetTick() - time_held > 500 && run_once) {
			mode = !mode;
			run_once = 0;
		}
	}
}

void encoder_handler() {
	int16_t encoder = __HAL_TIM_GET_COUNTER(&htim3);
	if (encoder < 1) {
		__HAL_TIM_SET_COUNTER(&htim3, 1);
		return;
	} else if (encoder > 100) {
		__HAL_TIM_SET_COUNTER(&htim3, 100);
		return;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_5) {
		if (mode) {
			single_flag = 1;
		}
	}
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
