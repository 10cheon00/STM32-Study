/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FS 12000
#define BUF_LEN 512      // DMA 이중버퍼 총 길이
#define FRAME_NSAMP 256  // 전송 단위(파이썬도 256 가정)
#define UART_BAUD 921600

// ==== 전송 예산(필수) ====
// 8N1에서 바이트당 10비트 사용, 샘플당 2바이트
#define UART_BITS_PER_BYTE 10
#define BYTES_PER_SAMPLE 2
#if (FS * BYTES_PER_SAMPLE * UART_BITS_PER_BYTE) > UART_BAUD
#error "UART_BAUD가 FS에 비해 낮습니다. FS를 낮추거나 UART_BAUD를 올리세요."
#endif

// ==== 디버그/가공 토글 ====
// 1: 가공 거의 없이 ADC-(2048)만 하고 <<4 하여 송출(가장 안전, 음성확인용)
// 0: 50Hz HPF + 보수적 스케일 + 소프트 리미팅(음질 개선용)
#define RAW_MODE 1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static uint16_t adc_buf[BUF_LEN];

static volatile uint8_t half_ready = 0;
static volatile uint8_t full_ready = 0;

// 상태(HPF용)
static float hpf_y = 0.0f;
static float x_prev = 0.0f;

// UART 전송 버퍼
static uint8_t tx_buf[6 + FRAME_NSAMP * 2];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

// 안전 포화
static inline int16_t sat16f(float v) {
  if (v > 32767.f) return 32767;
  if (v < -32768.f) return -32768;
  return (int16_t)v;
}

// ====== 가공 최소화 버전 (RAW_MODE=1) ======
//  - (in - 2048) << 4 만 적용 → WAV로 바로 들으면 “그냥 마이크 소리”가 나와야
//  정상
//  - 여기서도 이상하면 아날로그/샘플링/전송 문제임(가공 때문 아님).
// ====== 음질 개선 버전 (RAW_MODE=0) ======
//  - 1차 HPF(fc≈50 Hz, Fs=12kHz) + 보수 스케일 ×4 + 소프트 리미터(부드러운
//  무릎)

static uint16_t process_block_to_pcm(int16_t* out, const uint16_t* in,
                                     uint16_t N) {
#if RAW_MODE
  // ---- RAW 모드: 목소리 확인 최우선 ----
  // 12-bit(0..4095) → mid 제거 → 16-bit로 확장(<<4 ≒ ×16)하면 다소 큼 → ×8
  // 수준으로 맞춤 지터 없이 듣고자 살짝 낮춰서 ×8 사용
  for (uint16_t i = 0; i < N; i++) {
    int32_t dc_removed = (int32_t)in[i] - 2048;  // 중심 제거
    int32_t v = dc_removed << 3;  // ×8 (필요시 2~4로 더 낮춰도 됨)
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    out[i] = (int16_t)v;
  }
  return N;

#else
  // ---- 음질 개선 모드: HPF + 소프트 리미팅 ----
  const float HPF_ALPHA = 0.9742f;  // ≈ exp(-2π*50/12000)
  const float PRE_GAIN = 4.0f;      // 보수적 스케일
  const float LIM_T = 20000.0f;     // 무릎 시작점(헤드룸)
  const float LIM_SLOPE = 0.25f;    // 무릎 이후 기울기(0~1)

  float y_prev = hpf_y;
  float xprev = x_prev;

  for (uint16_t i = 0; i < N; i++) {
    float xc = (float)in[i] - 2048.0f;            // mid-bias 제거
    float y = HPF_ALPHA * (y_prev + xc - xprev);  // 1차 HPF
    xprev = xc;
    y_prev = y;

    float s = y * PRE_GAIN;

    // 소프트 리미팅
    float ab = (s >= 0.f) ? s : -s;
    if (ab > LIM_T) {
      float over = ab - LIM_T;
      ab = LIM_T + over * LIM_SLOPE;
      s = (s >= 0.f) ? ab : -ab;
    }
    out[i] = sat16f(s);
  }
  hpf_y = y_prev;
  x_prev = xprev;
  return N;
#endif
}

static void uart_send_frame(const int16_t* pcm, uint16_t N) {
  tx_buf[0] = 0x55;
  tx_buf[1] = 0xAA;
  tx_buf[2] = 'S';
  tx_buf[3] = '1';
  tx_buf[4] = (uint8_t)(N & 0xFF);
  tx_buf[5] = (uint8_t)(N >> 8);

  uint8_t* p = &tx_buf[6];
  for (uint16_t i = 0; i < N; i++) {
    int16_t s = pcm[i];
    *p++ = (uint8_t)(s & 0xFF);
    *p++ = (uint8_t)((s >> 8) & 0xFF);
  }
  HAL_UART_Transmit(&huart2, tx_buf, 6 + 2 * N, HAL_MAX_DELAY);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
  if (hadc->Instance == ADC1) half_ready = 1;
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
  if (hadc->Instance == ADC1) full_ready = 1;
}

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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
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
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, BUF_LEN);
  HAL_TIM_Base_Start(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int16_t pcm_frame[FRAME_NSAMP];

  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // Half-buffer 준비
    if (half_ready) {
      half_ready = 0;

      const uint16_t* src = &adc_buf[0];  // 하프버퍼 #0 (0..255)
      int16_t pcm_frame[FRAME_NSAMP];
      uint16_t produced = process_block_to_pcm(pcm_frame, src, FRAME_NSAMP);
      uart_send_frame(pcm_frame, produced);  // 딱 1프레임만 전송
    }

    if (full_ready) {
      full_ready = 0;

      const uint16_t* src = &adc_buf[BUF_LEN / 2];  // 하프버퍼 #1 (256..511)
      int16_t pcm_frame[FRAME_NSAMP];
      uint16_t produced = process_block_to_pcm(pcm_frame, src, FRAME_NSAMP);
      uart_send_frame(pcm_frame, produced);  // 딱 1프레임만 전송
    }
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
   */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {
  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data
   * Alignment and number of conversion)
   */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in
   * the sequencer and its sample time.
   */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 124;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 59;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {
  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 921600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
