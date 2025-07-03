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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "sd_card.h"
#include <stdarg.h>
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
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void UART_Printf(const char *fmt, ...) {
    char buff[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, sizeof(buff), fmt, args);
    HAL_UART_Transmit(&huart2, (uint8_t *)buff, strlen(buff), HAL_MAX_DELAY);
    va_end(args);
}

void init() {
    int code;
    UART_Printf("Ready!\r\n");

    code = SDCARD_Init();
    if (code < 0) {
        UART_Printf("SDCARD_Init() failed: code = %d\r\n", code);
        return;
    }

    UART_Printf("SDCARD_Init() done!\r\n");

    uint32_t blocksNum;
    code = SDCARD_GetBlocksNumber(&blocksNum);
    if (code < 0) {
        UART_Printf("SDCARD_GetBlocksNumber() failed: code = %d\r\n", code);
        return;
    }

    UART_Printf("SDCARD_GetBlocksNumber() done! blocksNum = %u (or %u Mb)\r\n",
                blocksNum, blocksNum / 2000 /* same as * 512 / 1000 / 1000 */);

    uint32_t startBlockAddr = 0x00ABCD;
    uint32_t blockAddr = startBlockAddr;
    uint8_t block[512];

    snprintf((char *)block, sizeof(block), "0x%08X", (int)blockAddr);

    code = SDCARD_WriteSingleBlock(blockAddr, block);
    if (code < 0) {
        UART_Printf("SDCARD_WriteSingleBlock() failed: code = %d\r\n", code);
        return;
    }
    UART_Printf("SDCARD_WriteSingleBlock(0x%08X, ...) done!\r\n", blockAddr);

    memset(block, 0, sizeof(block));

    code = SDCARD_ReadSingleBlock(blockAddr, block);
    if (code < 0) {
        UART_Printf("SDCARD_ReadSingleBlock() failed: code = %d\r\n", code);
        return;
    }

    UART_Printf("SDCARD_ReadSingleBlock(0x%08X, ...) done! block = "
                "\"%c%c%c%c%c%c%c%c%c%c...\"\r\n",
                blockAddr, block[0], block[1], block[2], block[3], block[4],
                block[5], block[6], block[7], block[8], block[9]);

    blockAddr = startBlockAddr + 1;
    code = SDCARD_WriteBegin(blockAddr);
    if (code < 0) {
        UART_Printf("SDCARD_WriteBegin() failed: code = %d\r\n", code);
        return;
    }
    UART_Printf("SDCARD_WriteBegin(0x%08X, ...) done!\r\n", blockAddr);

    for (int i = 0; i < 3; i++) {
        snprintf((char *)block, sizeof(block), "0x%08X", (int)blockAddr);

        code = SDCARD_WriteData(block);
        if (code < 0) {
            UART_Printf("SDCARD_WriteData() failed: code = %d\r\n", code);
            return;
        }

        UART_Printf("SDCARD_WriteData() done! blockAddr = %08X\r\n", blockAddr);
        blockAddr++;
    }

    code = SDCARD_WriteEnd();
    if (code < 0) {
        UART_Printf("SDCARD_WriteEnd() failed: code = %d\r\n", code);
        return;
    }
    UART_Printf("SDCARD_WriteEnd() done!\r\n");

    blockAddr = startBlockAddr + 1;
    code = SDCARD_ReadBegin(blockAddr);
    if (code < 0) {
        UART_Printf("SDCARD_ReadBegin() failed: code = %d\r\n", code);
        return;
    }
    UART_Printf("SDCARD_ReadBegin(0x%08X, ...) done!\r\n", blockAddr);

    for (int i = 0; i < 3; i++) {
        code = SDCARD_ReadData(block);
        if (code < 0) {
            UART_Printf("SDCARD_ReadData() failed: code = %d\r\n", code);
            return;
        }

        UART_Printf(
            "SDCARD_ReadData() done! block = \"%c%c%c%c%c%c%c%c%c%c...\"\r\n",
            block[0], block[1], block[2], block[3], block[4], block[5],
            block[6], block[7], block[8], block[9]);
    }

    code = SDCARD_ReadEnd();
    if (code < 0) {
        UART_Printf("SDCARD_ReadEnd() failed: code = %d\r\n", code);
        return;
    }
    UART_Printf("SDCARD_ReadEnd() done!\r\n");
}

void loop() { HAL_Delay(500); }
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_FATFS_Init();
    MX_USART2_UART_Init();
    /* USER CODE BEGIN 2 */

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    init();
    while (1) {
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
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV16;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

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
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */
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
    huart2.Init.BaudRate = 9600;
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
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);

    /*Configure GPIO pin : CS_Pin */
    GPIO_InitStruct.Pin = CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CS_GPIO_Port, &GPIO_InitStruct);

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
    /* User can add his own implementation to report the HAL error return state
     */
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
