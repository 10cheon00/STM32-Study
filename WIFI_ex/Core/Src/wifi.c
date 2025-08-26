#include "wifi.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const char* WIFI_CommandString[] = {[AT] = "AT",
                                    [ATE0] = "ATE0",
                                    [AT_CWMODE_CUR] = "AT+CWMODE_CUR",
                                    [AT_CWLAP] = "AT+CWLAP",
                                    [AT_CWJAP] = "AT+CWJAP",
                                    [AT_CWQAP] = "AT+CWQAP",
                                    [AT_CIFSR] = "AT+CIFSR",
                                    [AT_CIPSTATUS] = "AT+CIPSTATUS",
                                    [AT_CIPSTART] = "AT+CIPSTART",
                                    [AT_CIPSEND] = "AT+CIPSEND",
                                    [AT_CIPCLOSE] = "AT+CIPCLOSE"};

HAL_StatusTypeDef WIFI_Init(WIFI_HandleTypeDef* wifi, UART_HandleTypeDef* huart,
                            uint32_t timeoutTick,
                            WIFI_ATCommandSignatureTypeDef* commandQueue,
                            int commandQueueSize) {
  wifi->huart = huart;
  wifi->timeoutTick = timeoutTick;
  wifi->commandQueue = commandQueue;
  wifi->commandQueueSize = commandQueueSize;
  wifi->commandQueueIndex = 0;
  wifi->delayTick = WIFI_DEFAULT_DELAY_TICK;
  wifi->retryCount = 0;
}

void WIFI_GenerateCommand(WIFI_HandleTypeDef* wifi,
                          WIFI_ATCommandSignatureTypeDef* commandSignature) {
  // cmd to string
  int i, bufferSize = MAX_COMMAND_LEN, result;
  WIFI_ATCommandTypeDef cmdType = commandSignature->command;
  char* cmd = wifi->command;
  const char* cmdString = WIFI_CommandString[cmdType];
  char *equationStr = "=", *separatorStr = ",", *suffixStr = "\r\n\0";

  // 커맨드를 문자열로 변환
  // TODO: 인자를 문자열로 변환한 다음 처리하는
  //       문자열의 인덱스 갱신과정 및 포인터 관리가 매우 난해함
  result = appendString(cmd, &bufferSize, &S(cmdString));
  if (result != ARG_SUCCESS) {
    wifi->status = WIFI_STATUS_ERROR;
    return;
  }
  wifi->commandLen = MAX_COMMAND_LEN - bufferSize;

  for (i = 0; i < commandSignature->argumentSize; i++) {
    Arg* arg = &commandSignature->arguments[i];
    if (i == 0) {
      // 첫 인자 앞에는 '=' 삽입
      cmd = wifi->command + wifi->commandLen;
      result = appendString(cmd, &bufferSize, &S(equationStr));
      if (result != ARG_SUCCESS) {
        wifi->status = WIFI_STATUS_ERROR;
        return;
      }
      wifi->commandLen = MAX_COMMAND_LEN - bufferSize;
    }
    // 구분자 삽입
    if (i > 0) {
      cmd = wifi->command + wifi->commandLen;
      if (appendString(cmd, &bufferSize, &S(separatorStr)) != ARG_SUCCESS) {
        break;
      }
      wifi->commandLen = MAX_COMMAND_LEN - bufferSize;
    }

    // 가변인자 파싱, 삽입
    cmd = wifi->command + wifi->commandLen;
    if (arg->kind == ARG_QS) {
      result = appendQuoteString(cmd, &bufferSize, arg);
    } else if (arg->kind == ARG_S) {
      result = appendString(cmd, &bufferSize, arg);
    } else if (arg->kind == ARG_I) {
      result = appendInt(cmd, &bufferSize, arg);
    }
    if (result != ARG_SUCCESS) {
      break;
    }
    wifi->commandLen = MAX_COMMAND_LEN - bufferSize;
  }
  if (result != ARG_SUCCESS) {
    wifi->status = WIFI_STATUS_ERROR;
    return;
  }
  cmd = wifi->command + wifi->commandLen;
  result = appendString(cmd, &bufferSize, &S(suffixStr));
  if (result != ARG_SUCCESS) {
    wifi->status = WIFI_STATUS_ERROR;
    return;
  }
  wifi->commandLen = strlen(wifi->command);
}

void WIFI_SendCommand(WIFI_HandleTypeDef* wifi) {
  HAL_StatusTypeDef status;
  wifi->status = WIFI_STATUS_BUSY;
  wifi->lastTxTick = HAL_GetTick();
  status = HAL_UART_Transmit_DMA(wifi->huart, wifi->command, wifi->commandLen);
  status = HAL_UARTEx_ReceiveToIdle_DMA(wifi->huart, wifi->response,
                                        MAX_COMMAND_LEN);
}

void WIFI_ProcessError(WIFI_HandleTypeDef* wifi) {
  WIFI_ATCommandSignatureTypeDef atCommand = {AT, 0, NULL};
  WIFI_GenerateCommand(wifi, &atCommand);
  WIFI_SendCommand(wifi);
  wifi->commandQueueIndex = 0;
}

HAL_StatusTypeDef WIFI_Tick(WIFI_HandleTypeDef* wifi) {
  if (wifi->status == WIFI_STATUS_ERROR) {
    if (wifi->lastTxTick + wifi->delayTick < HAL_GetTick()) {
      // 에러 해결을 위한 AT명령어를 실행하기 위해 기다린 후 진행
      WIFI_ProcessError(wifi);
    }
  } else if (wifi->status == WIFI_STATUS_BUSY) {
    if (wifi->lastTxTick + wifi->timeoutTick < HAL_GetTick()) {
      // 재시도
      // TODO: 재시도 횟수를 제한할까?
      if (wifi->retryCount < 5) {
        wifi->retryCount++;
        WIFI_SendCommand(wifi);
      } else {
        wifi->status = WIFI_STATUS_ERROR;
      }
    }
  } else if (wifi->status == WIFI_STATUS_WAIT) {
    if (wifi->lastTxTick + wifi->delayTick < HAL_GetTick()) {
      // 다음 명령어를 요청할 수 있도록 준비상태로 변경
      wifi->status = WIFI_STATUS_READY;
    }
  } else if (wifi->status == WIFI_STATUS_READY) {
    // 커맨드 큐에서 다음 요청을 꺼내 수행
    if (wifi->commandQueueIndex < wifi->commandQueueSize) {
      WIFI_ATCommandSignatureTypeDef* nextCommand =
          &wifi->commandQueue[wifi->commandQueueIndex];
      WIFI_GenerateCommand(wifi, nextCommand);
      WIFI_SendCommand(wifi);
    }
  }
}

void WIFI_CommandSuccessCallback(WIFI_HandleTypeDef* wifi) {
  wifi->status = WIFI_STATUS_WAIT;
  wifi->lastTxTick = HAL_GetTick();
  wifi->commandQueueIndex++;
  wifi->retryCount = 0;
}

void WIFI_CommandErrorCallback(WIFI_HandleTypeDef* wifi) {
  wifi->status = WIFI_STATUS_ERROR;
}

void WIFI_ProcessRx(WIFI_HandleTypeDef* wifi) {
  if (strstr(wifi->response, BUSY_P) == NULL &&
      strstr(wifi->response, BUSY_S) == NULL) {
    if (strstr(wifi->response, "OK") != NULL) {
      WIFI_CommandSuccessCallback(wifi);
    } else {
      WIFI_CommandErrorCallback(wifi);
    }
  }
}

// TODO: 커맨드 큐 방식이 아니라 함수 호출식으로 할 수 있을까?
//       비동기 상태 관리를 하기 어렵다고 생각하는데
HAL_StatusTypeDef WIFI_AT(WIFI_HandleTypeDef* wifi) { return HAL_OK; }

HAL_StatusTypeDef WIFI_ATE0(WIFI_HandleTypeDef* wifi) { return HAL_OK; }

HAL_StatusTypeDef WIFI_CWMODE_STA(WIFI_HandleTypeDef* wifi) { return HAL_OK; }

HAL_StatusTypeDef WIFI_CWJAP(WIFI_HandleTypeDef* wifi, const char* ssid,
                             const char* pwd) {
  return HAL_OK;
}

HAL_StatusTypeDef WIFI_CIPMUX_Single(WIFI_HandleTypeDef* wifi) {
  return HAL_OK;
}

HAL_StatusTypeDef WIFI_CIPSTART_TCP(WIFI_HandleTypeDef* wifi, const char* host,
                                    uint16_t port) {
  return HAL_OK;
}

HAL_StatusTypeDef WIFI_CIPSEND(WIFI_HandleTypeDef* wifi, uint16_t len) {
  return HAL_OK;
}

HAL_StatusTypeDef WIFI_SendPayload(WIFI_HandleTypeDef* wifi, const void* data,
                                   uint16_t n) {
  return HAL_OK;
}

HAL_StatusTypeDef WIFI_CIPCLOSE(WIFI_HandleTypeDef* wifi) { return HAL_OK; }
