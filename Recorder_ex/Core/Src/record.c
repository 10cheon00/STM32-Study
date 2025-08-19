#include "record.h"

// ====== 가공 최소화 버전 (RAW_MODE=1) ======
//  - (in - 2048) << 4 만 적용 → WAV로 바로 들으면 “그냥 마이크 소리”가 나와야
//  정상
//  - 여기서도 이상하면 아날로그/샘플링/전송 문제임(가공 때문 아님).
// ====== 음질 개선 버전 (RAW_MODE=0) ======
//  - 1차 HPF(fc≈50 Hz, Fs=12kHz) + 보수 스케일 ×4 + 소프트 리미터(부드러운
//  무릎)
uint16_t process_block_to_pcm(int16_t* out, const uint16_t* in,
                                     uint16_t N) {
#if RAW_MODE
  // ---- RAW 모드: 목소리 확인 최우선 ----
  // 12-bit(0..4095) → mid 제거 → 16-bit로 확장(<<4 ≒ ×16)하면 다소 큼 → ×8
  // 수준으로 맞춤 지터 없이 듣고자 살짝 낮춰서 ×8 사용
  for (uint16_t i = 0; i < N; i++) {
    int32_t dc_removed = (int32_t)in[i] - 2048;  // 중심 제거
    int32_t v = dc_removed << 3;  // ×8 (필요시 2~4로 더 낮춰도 됨)
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    out[i] = (int16_t)v;
  }
  return N;

#else
  // ---- 음질 개선 모드: HPF + 소프트 리미팅 ----
  const float HPF_ALPHA = 0.9742f;  // ≈ exp(-2π*50/12000)
  const float PRE_GAIN = 4.0f;      // 보수적 스케일
  const float LIM_T = 20000.0f;     // 무릎 시작점(헤드룸)
  const float LIM_SLOPE = 0.25f;    // 무릎 이후 기울기(0~1)

  float y_prev = hpf_y;
  float xprev = x_prev;

  for (uint16_t i = 0; i < N; i++) {
    float xc = (float)in[i] - 2048.0f;            // mid-bias 제거
    float y = HPF_ALPHA * (y_prev + xc - xprev);  // 1차 HPF
    xprev = xc;
    y_prev = y;

    float s = y * PRE_GAIN;

    // 소프트 리미팅
    float ab = (s >= 0.f) ? s : -s;
    if (ab > LIM_T) {
      float over = ab - LIM_T;
      ab = LIM_T + over * LIM_SLOPE;
      s = (s >= 0.f) ? ab : -ab;
    }
    out[i] = sat16f(s);
  }
  hpf_y = y_prev;
  x_prev = xprev;
  return N;
#endif
}

void uart_send_frame(const int16_t* pcm, uint16_t N) {
  tx_buf[0] = 0x55;
  tx_buf[1] = 0xAA;
  tx_buf[2] = 'S';
  tx_buf[3] = '1';
  tx_buf[4] = (uint8_t)(N & 0xFF);
  tx_buf[5] = (uint8_t)(N >> 8);

  uint8_t* p = &tx_buf[6];
  for (uint16_t i = 0; i < N; i++) {
    int16_t s = pcm[i];
    *p++ = (uint8_t)(s & 0xFF);
    *p++ = (uint8_t)((s >> 8) & 0xFF);
  }
  HAL_UART_Transmit(&huart2, tx_buf, 6 + 2 * N, HAL_MAX_DELAY);
}