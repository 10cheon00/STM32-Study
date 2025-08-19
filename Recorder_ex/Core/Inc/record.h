#ifndef _RECORD_H_
#define _RECORD_H_

#include "stm32f4xx.h"

// ==== 디버그/가공 토글 ====
// 1: 가공 거의 없이 ADC-(2048)만 하고 <<4 하여 송출(가장 안전, 음성확인용)
// 0: 50Hz HPF + 보수적 스케일 + 소프트 리미팅(음질 개선용)
#define RAW_MODE 1

#define FS 44096
#define OSR 1
#define FRAME_NSAMP 256  // 전송 단위(파이썬도 256 가정)
#define UART_BAUD 921600

// ==== 전송 예산(필수) ====
// 8N1에서 바이트당 10비트 사용, 샘플당 2바이트
#define UART_BITS_PER_BYTE 10
#define BYTES_PER_SAMPLE 2
#if (FS * BYTES_PER_SAMPLE * UART_BITS_PER_BYTE) > UART_BAUD
#error "UART_BAUD가 FS에 비해 낮습니다. FS를 낮추거나 UART_BAUD를 올리세요."
#endif
#define BUF_LEN (FRAME_NSAMP*OSR*2)      // DMA 이중버퍼 총 길이

extern UART_HandleTypeDef huart2;

static uint16_t adc_buf[BUF_LEN];

// 상태(HPF용)
static float hpf_y = 0.0f;
static float x_prev = 0.0f;

// UART 전송 버퍼
static uint8_t tx_buf[6 + FRAME_NSAMP * 2];

uint16_t process_block_to_pcm(int16_t* out, const uint16_t* in,uint16_t N);
void uart_send_frame(const int16_t* pcm, uint16_t N);

#endif