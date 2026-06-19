/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "bmp.h"
#include "bmp_280_SPI.h"
#include <stdint.h>
#include "uart_ring_buffer.h"
#include "uart_dma_manager.h"
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
Bmp_280_Interface bmp_device;
SPI_BMP_CS_PIN cs_pin;
BMP280_StatusTypeDef bmp_status;
volatile int32_t temperature;
volatile uint32_t pressure;
uint32_t last_tick = 0;
uint8_t read_step = 0;
uint32_t test_counter = 0;
#define BMP280_READ_INTERVAL_MS 1000
uint8_t my_payload[100];

/* Define Ring Buffers for UART DMA Manager */

RingBuffer rx_ring_buffer;
RingBuffer tx_ring_buffer;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

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
  MX_DMA_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  uint32_t last_tick = 0;
  uint32_t test_counter = 0;
   if (BMP280_CS_PIN_INIT(&cs_pin, GPIOB, GPIO_PIN_0) != BMP280_OK)
  {
      bmp_status = BMP280_ERR_SPI;
      Error_Handler();
  }
  if (BMP280_SPI_Init(&bmp_device, &cs_pin) != BMP280_OK)
  {
      bmp_status = BMP280_ERR_SPI;
      Error_Handler();
  }
  
  if (BMP280_Init(&bmp_device) != BMP280_OK)
  {
      bmp_status = BMP280_ERR_SPI;
      Error_Handler();
  }
  if( BMP280_SetMode(&bmp_device, BMP280_MODE_NORMAL) != BMP280_OK)
  {
      bmp_status = BMP280_ERR_SPI;
      Error_Handler();
  }
  if (BMP280_SetOversampling(&bmp_device, BMP280_OSRS_T_2X, BMP280_OSRS_P_2X) != BMP280_OK)
  {
      bmp_status = BMP280_ERR_SPI;
      Error_Handler();
  }
  if (BMP280_SetConfig(&bmp_device, BMP280_STANDBY_250_MS, BMP280_FILTER_COEFF_4) != BMP280_OK)
  {
      bmp_status = BMP280_ERR_SPI;
      Error_Handler();
  }

  /* 1. Initialize the Ring Buffers */
  rb_init(&rx_ring_buffer);
  rb_init(&tx_ring_buffer);

  /* 2. Disable Hardware Flow Control */
  UART_Manager_EnableSoftwareFlowControl(false); 

  /* 3. Initialize the UART Manager */
  if (UART_Manager_Init(&huart2, &rx_ring_buffer, &tx_ring_buffer) != HAL_OK)
  {
      Error_Handler(); 
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (HAL_GetTick() - last_tick >= BMP280_READ_INTERVAL_MS) // 1 second interval
    {
      last_tick = HAL_GetTick();
      switch(read_step){
        case 0:
          bmp_status = BMP280_ReadTemperature_IT(&bmp_device);
          read_step = 1;
          break;
        case 1:
          temperature = BMP280_Convert_RawTemperature();
          bmp_status = BMP280_ReadPressure_IT(&bmp_device);
          read_step = 2;
          break;
        case 2:
          pressure = BMP280_Convert_RawPressure();
          read_step = 0;
          test_counter++;
          break;
      }
      /* Використовуємо цілі числа замість float, щоб уникнути проблем з printf_float на STM32 */
      snprintf((char*)my_payload, sizeof(my_payload), 
      "Temp: %d.%02d C, Pressure: %lu.%02lu hPa\r\n", 
      (int)(temperature / 100), (int)(temperature % 100), 
      (uint32_t)(pressure / 100), (uint32_t)(pressure % 100));
      
      /* 4. Transmit data to UART */
      rb_push_array(&tx_ring_buffer, my_payload, strlen((char*)my_payload));
    }
    
    /* 5. Call UART Manager task to process buffers */
    UART_Manager_Task();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C2)
  {
      BMP280_Rx_CpltCallback();
  }
}
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    BMP280_SPI_END_TRANSACTION(bmp_device.intf_ptr);
    BMP280_Rx_CpltCallback();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
