#include "matrix.h"

void matrix_init(SPI_HandleTypeDef *hspi1) {
    // 셧다운 모드 탈출
    write_matrix(hspi1, LED_MATRIX_SHUTDOWN_MODE_ADDRESS |
                            LED_MATRIX_SHUTDOWN_MODE_DISABLE);
    // 디코드 모드 해제
    write_matrix(hspi1, LED_MATRIX_DECODE_MODE_ADDRESS |
                            LED_MATRIX_DECODE_MODE_DISABLE);
    // 디스플레이 테스트 모드 해제
    write_matrix(hspi1, LED_MATRIX_DISPLAY_TEST_ADDRESS |
                            LED_MATRIX_DISPLAY_TEST_DISABLE);
    // 스캔 모드 활성화
    write_matrix(hspi1,
                 LED_MATRIX_SCAN_MODE_ADDRESS | LED_MATRIX_SCAN_MODE_ALL);
    // 밝기 중간으로 설정
    write_matrix(hspi1,
                 LED_MATRIX_INTENSITY_ADDRESS | LED_MATRIX_INTENSITY_VALUE(1));
}

void write_matrix(SPI_HandleTypeDef *hspi1, uint16_t data) {
    HAL_GPIO_WritePin(GPIOA, PIN_CS, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi1, (uint8_t *)&data, 1, 0xFF);
    HAL_GPIO_WritePin(GPIOA, PIN_CS, GPIO_PIN_SET);
}
