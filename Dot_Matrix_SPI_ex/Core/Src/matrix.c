#include "matrix.h"

void LED_Matrix_Init(SPI_HandleTypeDef *hspi) {
    // 셧다운 모드 탈출
    LED_Matrix_Send(hspi, LED_MATRIX_SHUTDOWN_MODE_ADDRESS |
                              LED_MATRIX_SHUTDOWN_MODE_DISABLE);
    // 디코드 모드 해제
    LED_Matrix_Send(hspi, LED_MATRIX_DECODE_MODE_ADDRESS |
                              LED_MATRIX_DECODE_MODE_DISABLE);
    // 디스플레이 테스트 모드 해제
    LED_Matrix_Send(hspi, LED_MATRIX_DISPLAY_TEST_ADDRESS |
                              LED_MATRIX_DISPLAY_TEST_DISABLE);
    // 스캔 모드 활성화
    LED_Matrix_Send(hspi,
                    LED_MATRIX_SCAN_MODE_ADDRESS | LED_MATRIX_SCAN_MODE_ALL);
    // 밝기 중간으로 설정
    LED_Matrix_Send(hspi, LED_MATRIX_INTENSITY_ADDRESS |
                              LED_MATRIX_INTENSITY_VALUE(1));
}

void LED_Matrix_Clear(SPI_HandleTypeDef *hspi1) {
    uint8_t matrix[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    LED_Matrix_Print(hspi1, matrix);
}

void LED_Matrix_Send(SPI_HandleTypeDef *hspi1, uint16_t data) {
    HAL_GPIO_WritePin(GPIOA, PIN_CS, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi1, (uint8_t *)&data, 1, 0xFF);
    HAL_GPIO_WritePin(GPIOA, PIN_CS, GPIO_PIN_SET);
}

void LED_Matrix_Print(SPI_HandleTypeDef *hspi, uint8_t *matrix) {
    for (int i = 0; i < 8; i++) {
        uint16_t addr = 0x0100 * (i + 1);
        LED_Matrix_Send(hspi, addr | matrix[i]);
    }
}
