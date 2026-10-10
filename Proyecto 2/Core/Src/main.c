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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

uint8_t dato;

volatile uint8_t comando = 0;
volatile uint8_t estado = 0;
volatile uint8_t indiceSPI = 0;
volatile uint32_t ultimoCaracter = 0;

char textoSPI[24];

uint8_t datosI2C[2];
uint16_t valorADC = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void menu(void);
void leerpot(void);
void procesarSPI (void);
void enviarSPI(uint8_t led, uint16_t tiempo);
void configurarTimer(uint16_t tiempo);
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
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_UART_Receive_IT(&huart2, &dato, 1);
  menu();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // OPCION 1: CONTROLAR LEDS POR SPI
		      if (comando == 1)
		      {
		          comando = 0;

		          char mensaje[] =
		              "\r\n===== CONTROL DE LEDS =====\r\n"
		              "1. LED Rojo\r\n"
		              "2. LED Verde\r\n"
		              "3. LED Azul\r\n"
		              "\r\nFormato: LED,tiempo(ms)\r\n"
		              "Ejemplo: 1,500\r\n"
		              "Ingrese comando: ";

		          HAL_UART_Transmit(&huart2,(uint8_t*)mensaje,strlen(mensaje), 100);
		      }

		      // OPCION 2: LECTURA I2C
		      if (comando == 2)
		      {
		          comando = 0;
		          leerpot();
		          menu();
		      }

		      // DETECTAR FINAL DEL COMANDO
		      if (estado == 1 && indiceSPI > 0)
		      {
		          if ((uint32_t)(HAL_GetTick() -ultimoCaracter) >= 150)
		          {
		              estado = 2;
		          }
		      }

		      // EJECUTAR COMANDO SPI Y ACTUALIZAR TIMER
		      if (estado == 2)
		      {
		          estado = 3;
		          textoSPI[indiceSPI] = '\0';
		          procesarSPI();
		          indiceSPI = 0;
		          estado = 0;
		          menu();
		      }

		      // COMANDO DEMASIADO LARGO
		      if (estado == 4)
		      {
		          estado = 3;

		          char mensaje[] =
		              "\r\nComando muy largo\r\n";

		          HAL_UART_Transmit(&huart2,(uint8_t*)mensaje,strlen(mensaje), 100);
		          indiceSPI = 0;
		          estado = 0;

		          menu();
		      }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 5000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|led_azul_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 LD2_Pin led_azul_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_4|LD2_Pin|led_azul_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // MENU PRINCIPAL
        if (estado == 0 && comando == 0)
        {
            if (dato == '1')
            {
                estado = 1;
                indiceSPI = 0;
                comando = 1;
            }
            else if (dato == '2')
            {
                comando = 2;
            }
        }

        // RECIBIR COMANDO SPI
        else if (estado == 1)
        {
            if (dato == '\r' || dato == '\n')
            {
                if (indiceSPI > 0)
                {
                    estado = 2;
                }
            }
            else if (indiceSPI < sizeof(textoSPI) - 1)
            {
                textoSPI[indiceSPI] = dato;
                indiceSPI++;

                ultimoCaracter = HAL_GetTick();
            }
            else
            {
                estado = 4;
            }
        }

        HAL_UART_Receive_IT(&huart2, &dato, 1);
    }
}

void menu(void)
{
    char mensaje[] =
        "\r\n===== MENU PRINCIPAL =====\r\n"
        "1. Controlar dispositivo SPI\r\n"
        "2. Obtener medicion de sensor I2C\r\n"
        "Seleccione una opcion: ";

    HAL_UART_Transmit(&huart2, (uint8_t*)mensaje,strlen(mensaje), 100);
}

void leerpot(void)
{
    uint8_t solicitud = 1;

    // Solicitar una medicion al ESP32
    HAL_StatusTypeDef resultado;

    resultado = HAL_I2C_Master_Transmit(&hi2c1,(0x55 << 1),&solicitud,1,100);

    if (resultado != HAL_OK)
    {
        char mensaje[] =
            "\r\nError al solicitar ADC\r\n";

        HAL_UART_Transmit(&huart2,(uint8_t*)mensaje,strlen(mensaje), 100);
        return;
    }

    // Dar tiempo a preparar los dos bytes
    HAL_Delay(5);

    // Recibir la medicion
    resultado = HAL_I2C_Master_Receive( &hi2c1,(0x55 << 1),datosI2C,2,100);

    if (resultado == HAL_OK)
    {
        valorADC =((uint16_t)datosI2C[0] << 8)| datosI2C[1];

        if (valorADC <= 4095)
        {
            char mensaje[60];

            snprintf(mensaje,
                     sizeof(mensaje),
                     "\r\nADC: %u\r\n",
                     valorADC);

            HAL_UART_Transmit(&huart2,(uint8_t*)mensaje,strlen(mensaje), 100);
        }
        else
        {
            char mensaje[] =
                "\r\nLectura ADC invalida\r\n";

            HAL_UART_Transmit(&huart2,(uint8_t*)mensaje, strlen(mensaje), 100);
        }
    }
    else
    {
        char mensaje[] =
            "\r\nError de comunicacion I2C\r\n";

        HAL_UART_Transmit(&huart2,(uint8_t*)mensaje,strlen(mensaje), 100);
    }
}



void enviarSPI(uint8_t led, uint16_t tiempo)
{
    uint8_t datosSPI[4];

    datosSPI[0] = led;
    datosSPI[1] = (uint8_t)(tiempo >> 8);
    datosSPI[2] = (uint8_t)(tiempo & 0xFF);

    datosSPI[3] =datosSPI[0] ^ datosSPI[1] ^datosSPI[2] ^ 0xA5;


    // Activar CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(10);

    HAL_StatusTypeDef resultado =HAL_SPI_Transmit(&hspi1, datosSPI, 4, 1000);
    // Desactivar CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    if (resultado == HAL_OK)
    {
        char mensaje[] = "\r\nComando SPI transmitido\r\n";

        HAL_UART_Transmit(&huart2, (uint8_t*)mensaje,strlen(mensaje), 100);
    }
    else
    {
        char mensaje[] = "\r\nError de transmision SPI\r\n";

        HAL_UART_Transmit(&huart2,(uint8_t*)mensaje, strlen(mensaje), 100);
    }
}


void procesarSPI(void)
{
    unsigned int led;
    unsigned long tiempo;
    char extra;

    int resultado = sscanf(textoSPI,"%u,%lu %c",&led,&tiempo,&extra);

    if (resultado == 2 &&led >= 1 && led <= 3 &&tiempo >= 1 && tiempo <= 60000)
    {
        enviarSPI((uint8_t)led, (uint16_t)tiempo);

        configurarTimer((uint16_t)tiempo);
    }

    else
    {
        char mensaje[] =
            "\r\nComando invalido\r\n"
            "Ejemplo: 1,500\r\n";

        HAL_UART_Transmit(&huart2, (uint8_t*)mensaje,strlen(mensaje), 100);
    }
}



void configurarTimer(uint16_t tiempo)
{
    uint32_t alto = (uint32_t)tiempo * 10;
    uint32_t periodo = alto * 2;
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE (&htim2, TIM_CHANNEL_1,alto);
    __HAL_TIM_SET_AUTORELOAD (&htim2, periodo -1);
    __HAL_TIM_SET_COUNTER (&htim2, 0);
    htim2.Instance->EGR = TIM_EGR_UG;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
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
