#ifndef _UDA1334A_H_
#define _UDA1334A_H_

#include "stm32f4xx.h"

#define SAMPLE_RATE I2S_AUDIOFREQ_44K  // 44.1 kHz
#define SINE_TABLE_LEN 2048
#define FRAMES_PER_HALF 512           // half-buffer 프레임 수
#define STEREO 2
#define BIT_DEPTH 16
#define AMP 8000  // 16-bit 범위 내 적당한 진폭(클리핑 여유)
#define TONE_HZ 1000.0f

int16_t* UDA_Init();
void UDA_BuildSineTable(void);
void UDA_FillHalf(int16_t* buf);
void UDA_SetToneByADC(uint16_t adc_tone);
void UDA_SetVolumeByADC(uint16_t adc_volume);
#endif