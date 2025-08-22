#include "uda1334a.h"

#include <math.h>

static int16_t sine_table[TABLE_LEN];
/* 스테레오 인터리브 버퍼(좌,우,좌,우,...) */
static int16_t tx_buf[TABLE_LEN * STEREO];
/* 전역/정적 상태 */
static uint32_t g_gain_q15 = 0;  // 0..32767

// 0부터 2pi만큼의 사인파를 테이블 길이만큼 잘라 테이블 구성
int16_t* UDA_BuildSineTable(void) {
  for (uint32_t n = 0; n < TABLE_LEN; n++) {
    float ph = 2.0f * (float)M_PI * (float)n / (float)TABLE_LEN;
    sine_table[n] = (int16_t)lroundf(AMP * sinf(ph));
  }
  return tx_buf;
}

/* ---- ADC → 주파수 매핑 ---- */
static inline float map_adc_to_freq(uint16_t adc12, int log_scale) {
  float x = (float)adc12 / 4095.0f;  // 0..1
  if (log_scale) {
    /* 옥타브 느낌의 로그 매핑(권장) */
    return UDA_FMIN_HZ * powf(UDA_FMAX_HZ / UDA_FMIN_HZ, x);
  } else {
    /* 선형 매핑 */
    return UDA_FMIN_HZ + (UDA_FMAX_HZ - UDA_FMIN_HZ) * x;
  }
}

static inline float map_adc_to_volume(uint16_t adc12) {
  float x = (float)adc12 / 4095.0f;  // 0..1
  float dB = -60.0f + 60.0f * x;     // -60..0 dB
  return powf(10.0f, dB / 20.0f);    // 0.001..1.0
}

/* ---- ADC값으로 톤 선택 → 경계 연속 프레임 생성 ---- */
void UDA_BuildFrameFromADC(uint16_t adc12, uint16_t adc_volume, int log_scale,
                           float* out_freq_actual) {
  const float Fs = (float)I2S_SAMPLE_RATE;  // 예: 44100
  const uint32_t N = (uint32_t)TABLE_LEN;   // 예: 256(=2^m 권장)
  const float f = 440.0f;                   // 고정 톤

  /* 버퍼 경계 연속: N프레임에 K사이클이 정확히 들어가게 양자화 */
  uint32_t K = (uint32_t)lroundf(f * (float)N / Fs);
  if (K == 0) K = 1;
  if (K >= N) K = N - 1;

  /* 실제 재생 주파수(양자화 후) 리포트 */
  if (out_freq_actual) {
    *out_freq_actual = ((float)K * Fs) / (float)N;
  }

  /* 프레임 채우기 (L,R 인터리브). 테이블 진폭(AMP) 그대로 사용 */
  for (uint32_t n = 0; n < N; n++) {
    // N이 2^m이면 & (N-1), 아니면 % N 사용
    uint32_t i = (uint32_t)((uint64_t)K * n) & (N - 1);
    int16_t s = sine_table[i];  // build_sine_table()로 미리 생성된 값
    tx_buf[2 * n + 0] = s;      // Left
    tx_buf[2 * n + 1] = s;      // Right
  }
}