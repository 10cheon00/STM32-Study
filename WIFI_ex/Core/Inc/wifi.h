#ifndef WIFI_H_
#define WIFI_H_

#include <stdbool.h>

#include "stm32f4xx.h"
#include "variable_arg.h"

#define MAX_COMMAND_LEN 1024

typedef enum {
  AT = 0,
  ATE0,
  AT_CWMODE_CUR,
  AT_CWLAP,
  AT_CWJAP,
  AT_CWQAP,
  AT_CIFSR,
  AT_CIPSTATUS,
  AT_CIPSTART,
  AT_CIPSEND,
  AT_CIPCLOSE
} WIFI_ATCommandTypeDef;

typedef enum {
  WIFI_STATUS_READY,
  WIFI_STATUS_BUSY,
  WIFI_STATUS_ERROR,
  WIFI_STATUS_WAIT,
} WIFI_StatusTypeDef;

#define BUSY_P "busy p..."
#define BUSY_S "busy s..."

#define WIFI_DEFAULT_TIMEOUT_TICK 1000
#define WIFI_DEFAULT_DELAY_TICK 1000

// 사용자는 이 구조체에 인자를 담고, 이 구조체를 큐에 담아 전달하면
// WIFI_Tick() 함수에서 큐에 들어있는 이 구조체 인스턴스를 꺼내
// 요청을 수행한다.
typedef struct {
  WIFI_ATCommandTypeDef command;
  int argumentSize;
  Arg *arguments;
} WIFI_ATCommandSignatureTypeDef;

// 명령어는 UART전송 직전에 생성하기
// 명령어 생성에 필요한 인자는 미리 정의된 구조체에 담아두고, 생성 시기에
// 사용하기 인자는 union으로 선언된 타입에 저장,
//  그 인자가 어떤 타입인지 명시한 구조체가 인자를 감싼다.
// 빌드 시기에 구조체 배열을 순회하여 명령어 생성에 필요한 인자 꺼내기
typedef struct {
  UART_HandleTypeDef *huart;
  volatile WIFI_StatusTypeDef status;
  WIFI_ATCommandSignatureTypeDef *commandQueue;
  uint32_t commandQueueSize;
  uint32_t commandQueueIndex;
  char command[MAX_COMMAND_LEN];
  uint32_t commandLen;
  char response[MAX_COMMAND_LEN];
  uint32_t responseLen;
  uint32_t lastTxTick;
  uint32_t timeoutTick;
  uint32_t delayTick; // esp모듈이 다음 명령을 수행할 수 있도록 기다리는 시간
  uint32_t retryCount;
} WIFI_HandleTypeDef;

HAL_StatusTypeDef WIFI_Init(WIFI_HandleTypeDef *wifi, UART_HandleTypeDef *huart,
                            uint32_t timeoutTick,
                            WIFI_ATCommandSignatureTypeDef *commandQueue,
                            int commandQueueSize);

HAL_StatusTypeDef WIFI_Tick(WIFI_HandleTypeDef *wifi);

void WIFI_ProcessRx(WIFI_HandleTypeDef *wifi);

HAL_StatusTypeDef WIFI_AT(WIFI_HandleTypeDef *wifi);

HAL_StatusTypeDef WIFI_ATE0(WIFI_HandleTypeDef *wifi);

HAL_StatusTypeDef WIFI_CWMODE_STA(WIFI_HandleTypeDef *wifi);  // CWMODE_CUR=1

HAL_StatusTypeDef WIFI_CWJAP(WIFI_HandleTypeDef *wifi, const char *ssid,
                             const char *pwd);

HAL_StatusTypeDef WIFI_CIPMUX_Single(WIFI_HandleTypeDef *wifi);  // CIPMUX=0

HAL_StatusTypeDef WIFI_CIPSTART_TCP(WIFI_HandleTypeDef *wifi, const char *host,
                                    uint16_t port);

HAL_StatusTypeDef WIFI_CIPSEND(WIFI_HandleTypeDef *wifi,
                               uint16_t len);  // '>' 대기 단계 진입

HAL_StatusTypeDef WIFI_SendPayload(WIFI_HandleTypeDef *wifi, const void *data,
                                   uint16_t n);

HAL_StatusTypeDef WIFI_CIPCLOSE(WIFI_HandleTypeDef *wifi);

#endif