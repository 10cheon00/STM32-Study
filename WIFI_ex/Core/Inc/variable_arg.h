#ifndef WIFI_ARG_H_
#define WIFI_ARG_H_

#include "stm32f4xx.h"

typedef enum {
  ARG_SUCCESS = 0,
  ARG_FAIL = -1,
} Arg_StatusTypeDef;

// ---- 공용 타입: 어떤 타입인지 나타내는 태그 + 값 ----
typedef enum { ARG_END=0, ARG_QS, ARG_S, ARG_I, ARG_U, ARG_BOOL } ArgKind;
typedef struct {
  ArgKind kind;
  union { const char *s; int i; unsigned u; uint8_t b; } v;
} Arg;

// 생성 매크로(가독성↑)
#define QS(x)  ((Arg){ARG_QS,  .v.s=(x)})   // "따옴표" 붙여 출력할 문자열
#define S(x)   ((Arg){ARG_S,   .v.s=(x)})   // 그대로 출력할 문자열
#define I(x)   ((Arg){ARG_I,   .v.i=(x)})
#define U(x)   ((Arg){ARG_U,   .v.u=(x)})
#define B(x)   ((Arg){ARG_BOOL,.v.b=(x)})
#define END    ((Arg){ARG_END})

int appendQuoteString(char* dest, int* bufferSize, Arg* arg);
int appendString(char* dest, int* bufferSize, Arg* arg);
int appendInt(char* dest, int* bufferSize, Arg* arg);

#endif