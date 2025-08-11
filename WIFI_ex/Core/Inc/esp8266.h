#ifndef ESP8266_H_
#define ESP8266_H_

#include <stdarg.h>
#include "stm32f4xx.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

static void log2(const char *fmt, ...) {
    char buf[256];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 100);
}

/* ===== 유틸: 일정 시간 동안 1바이트씩 수신 =====
   - 전체 window_ms 동안 1바이트씩 non-blocking(짧은 timeout)으로 모읍니다 */
static uint32_t uart_recv_window(UART_HandleTypeDef *huart, uint8_t *buf,
                                 uint32_t maxlen, uint32_t window_ms) {
    uint32_t start = HAL_GetTick();
    uint32_t i = 0;
    while ((HAL_GetTick() - start) < window_ms && i < maxlen) {
        uint8_t ch;
        if (HAL_UART_Receive(huart, &ch, 1, 10) == HAL_OK) { // 10ms 대기
            buf[i++] = ch;
        }
        // 받는 즉시 다음 바이트 대기(시간 창 안에서만)
    }
    return i;
}

/* ===== ESP8266 AT 핑 테스트 ===== */
static void test_esp8266(void) {
    const char *cmd = "AT\r\n";
    uint8_t rx[256];
    memset(rx, 0, sizeof(rx));

    log2("\r\n[TEST] Send to ESP8266: %s", "AT\\r\\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, (uint16_t)strlen(cmd), 200);

    // 응답을 600ms 동안 수집(필요하면 늘리세요)
    uint32_t n = uart_recv_window(&huart1, rx, sizeof(rx)-1, 600);

    if (n == 0) {
        log2("[TEST] No response (timeout)\r\n");
        return;
    }

    // PC로 원문 덤프
    log2("[TEST] Raw %lu bytes from ESP8266:\r\n", (unsigned long)n);
    HAL_UART_Transmit(&huart2, rx, (uint16_t)n, 200);
    log2("\r\n");

    // "OK" 포함 여부로 간단 판정
    if (strstr((char*)rx, "OK")) {
        log2("[TEST] Result: OK ✅\r\n");
    } else {
        log2("[TEST] Result: Unknown/ERROR ❌\r\n");
    }
}

#endif