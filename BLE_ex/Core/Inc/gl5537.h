#ifndef GL5537_H_
#define GL5537_H_

#include "stm32f4xx.h"

#define ADC_FS   4095.0f   // 12-bit
#define VREF     3.3f      // ADC 참조(=VDDA)
#define VCC      3.3f      // 분배기 전원 (VREF와 같게 권장)
#define R_FIX    10000.0f  // 10kΩ
#define NAN      -1

typedef struct {
    float A;      // R = A * Lux^(-gamma) 의 A
    float gamma;  // gamma (>0)
} LDRCal;

/* --- 1) ADC -> 전압 --- */
static inline float adc_to_v(float adc_raw) {
    float v = (adc_raw / ADC_FS) * VREF;
    return v;
}

/* --- 2) 전압 -> R_LDR (케이스 A) ---
   R_LDR = R_FIX * (VCC - Vadc) / Vadc
*/
static inline float v_to_r_ldr_caseA(float v_adc) {
    const float eps = 1e-4f;
    if (v_adc < eps) v_adc = eps;
    if (v_adc > (VCC - eps)) v_adc = (VCC - eps);
    return R_FIX * (VCC - v_adc) / v_adc;
}

/* --- 3) R_LDR -> Lux (보정값 필요) ---
   Lux = (A / R)^(1/gamma)
*/
static inline float r_to_lux(float r_ldr, LDRCal c) {
    if (r_ldr <= 0.0f || c.A <= 0.0f || c.gamma <= 0.0f) return NAN;
    return powf((c.A / r_ldr), (1.0f / c.gamma));
}

/* --- (옵션) 두 점으로 보정 파라미터 구하기 ---
   입력: (L1, L2) [lux], (N1, N2) [ADC]  ->  보정값 A, gamma 반환
*/
static inline LDRCal calibrate_two_points(float L1, float N1, float L2, float N2) {
    // 1) N -> V -> R
    float V1 = adc_to_v(N1), V2 = adc_to_v(N2);
    float R1 = v_to_r_ldr_caseA(V1), R2 = v_to_r_ldr_caseA(V2);

    // 2) gamma, A
    LDRCal c = { .A = NAN, .gamma = NAN };
    if (L1 > 0 && L2 > 0 && R1 > 0 && R2 > 0 && L1 != L2) {
        float g = (logf(R1) - logf(R2)) / (logf(L2) - logf(L1)); // gamma
        float A = R1 * powf(L1, g);
        c.A = A; c.gamma = g;
    }
    return c;
}

/* --- 런타임 변환: ADC 읽고 Lux/밝기(0~1) 계산 --- */
typedef struct {
    float v_adc;      // [V]
    float r_ldr;      // [ohm]
    float brightness; // 0~1 (케이스 A: Vadc/Vcc)
    float lux;        // 보정값 있을 때만 유효
} Brightness;

Brightness convert_from_adc(uint16_t adc_raw, const LDRCal* cal_opt) {
    Brightness out;
    out.v_adc = adc_to_v(adc_raw);
    out.r_ldr = v_to_r_ldr_caseA(out.v_adc);
    // 케이스 A는 전압이 곧 밝기 비율
    float b = out.v_adc / VCC;
    if (b < 0) b = 0; if (b > 1) b = 1;
    out.brightness = b;
    out.lux = (cal_opt) ? r_to_lux(out.r_ldr, *cal_opt) : NAN;
    return out;
}

#endif