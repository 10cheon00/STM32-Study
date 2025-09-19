#include "uda1334a.h"

#include <math.h>

static int16_t sine_table[SINE_TABLE_LEN + 1];  // 보간을 위해 1칸을 추가
static int16_t tx_buf[FRAMES_PER_HALF * STEREO * 2];

static float freq;
static float volume;

// 상위 16비트를 정수로, 하위 16비트를 소수로 활용 = Q16.16
#define FRAC 16
static uint32_t phase;
static uint32_t step;

int16_t* UDA_Init() { 
  UDA_BuildSineTable();
  UDA_SetToneByADC(TONE_HZ);
  UDA_SetVolumeByADC(2048);
  UDA_FillHalf(&tx_buf[0]);
  UDA_FillHalf(&tx_buf[FRAMES_PER_HALF*STEREO]);

  return tx_buf;
 }

// 0부터 2pi만큼의 사인파를 테이블 길이만큼 잘라 테이블 구성
void UDA_BuildSineTable(void) {
  for (uint32_t n = 0; n < SINE_TABLE_LEN; n++) {
    float ph = 2.0f * (float)M_PI * ((float)n / (float)SINE_TABLE_LEN);
    sine_table[n] = (int16_t)lroundf(sinf(ph) * AMP);
  }
  sine_table[SINE_TABLE_LEN] = sine_table[0];
  phase = 0;
  return tx_buf;
}

int16_t calculateSampleOut(uint32_t* phase, uint32_t step) {
  // 위상 이동
	*phase += step;
	*phase &= (((uint32_t)SINE_TABLE_LEN << FRAC) - 1);
  
	// 샘플값 선형 보간
	const uint32_t index = (*phase) >> FRAC;
	int16_t v1 = sine_table[index];
	int16_t v2 = sine_table[index+1];
  uint32_t frac = (*phase) & 0xFFFF;
  int32_t out = ( v1 * (int32_t)(0x10000 - frac) + v2 * (int32_t)frac ) >> FRAC;

  // 샘플값 클리핑
	if (out > 32767) out = 32767;
	if (out < -32768) out = -32768;

	return (int16_t)out;
}

void UDA_FillHalf(int16_t* buf) {
  if (step < 1) step = 1;
  for (int i = 0; i < FRAMES_PER_HALF; i++) {
    int16_t s = calculateSampleOut(&phase, step) * volume;
    /* Left channel */
    buf[STEREO * i + 0] = s;
    /* Right channel */
    buf[STEREO * i + 1] = s;
  }
}

void UDA_SetToneByADC(uint16_t adc_tone) {
  uint16_t note = ((float)adc_tone / 4096.0f) * 12;
  freq = TONE_HZ * powf(2, (float)note / 12);
  step = (uint32_t)lroundf((freq * (float)SINE_TABLE_LEN / (float)SAMPLE_RATE) * 65536.0f);
}

void UDA_SetVolumeByADC(uint16_t adc_volume) {
  volume = ((float)adc_volume / 4096.0f);
}