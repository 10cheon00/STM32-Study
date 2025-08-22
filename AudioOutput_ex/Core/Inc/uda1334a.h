#ifndef _UDA1334A_H_
#define _UDA1334A_H_

#include "stm32f4xx.h"

#define I2S_SAMPLE_RATE 44100  // 48 kHz
#define TONE_HZ 440.00f        // A4 (원하면 바꾸세요)
#define TABLE_LEN 256          // 사인 테이블 길이
#define AMP 24000              // 16-bit 범위 내 적당한 진폭(클리핑 여유)
#define STEREO 2
/* ---- 포텐셔미터 → 주파수 범위 ---- */
#define UDA_FMIN_HZ     110.0f     // A2
#define UDA_FMAX_HZ     1760.0f    // A6


int16_t* UDA_BuildSineTable(void);

/* ADC값(0..4095)으로 톤을 선택해 "한 프레임 버퍼(LRLR...)" 생성
   - log_scale=0: 선형 매핑, 1: 로그(옥타브) 매핑
   - out_freq_actual != NULL 이면 실제 재생될 주파수(=K*Fs/N)를 돌려줌
   - 반환: 내부 정적 버퍼 포인터(int16_t*), 크기는 (TABLE_LEN*STEREO) 샘플
*/
void UDA_BuildFrameFromADC(uint16_t adc_tone, uint16_t adc_volume, int log_scale,
                                  float* out_freq_actual);

#endif