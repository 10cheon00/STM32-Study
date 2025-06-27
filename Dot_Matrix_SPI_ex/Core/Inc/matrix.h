#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "stm32f1xx_hal.h"

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define PIN_CS GPIO_PIN_6
#define LED_MATRIX_SHUTDOWN_MODE_ADDRESS 0x0C00
#define LED_MATRIX_DECODE_MODE_ADDRESS 0x0900
#define LED_MATRIX_SCAN_MODE_ADDRESS 0x0B00
#define LED_MATRIX_DISPLAY_TEST_ADDRESS 0x0F00
#define LED_MATRIX_INTENSITY_ADDRESS 0x0A00

#define LED_MATRIX_SHUTDOWN_MODE_ENABLE 0x0000
#define LED_MATRIX_SHUTDOWN_MODE_DISABLE 0x0001
#define LED_MATRIX_DECODE_MODE_DISABLE 0x0000
#define LED_MATRIX_SCAN_MODE_ALL 0x0007
#define LED_MATRIX_DISPLAY_TEST_DISABLE 0x0000
#define LED_MATRIX_DISPLAY_TEST_ENABLE 0x0001
/**
 * Intensity value range is [1,16).
 * parameter will be extended to [1, 32) with step 2
 */
#define LED_MATRIX_INTENSITY_VALUE(X) (MAX(0, MIN((2 * X - 1), 31)))

#define LED_MATRIX_ROW_ADDRESS 0x0100

void LED_Matrix_Init(SPI_HandleTypeDef *hspi);

void LED_Matrix_Clear(SPI_HandleTypeDef *hspi);

void LED_Matrix_Send(SPI_HandleTypeDef *hspi, uint16_t data);

void LED_Matrix_Print(SPI_HandleTypeDef *hspi, uint8_t *matrix);
#endif /* _MATRIX_H_*/