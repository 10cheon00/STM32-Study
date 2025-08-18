#include "wifi.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "variable_arg.h"

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

HAL_StatusTypeDef WIFI_Init(WIFI_HandleTypeDef* wifi) {}

void WIFI_GenerateCommand(WIFI_HandleTypeDef* wifi,
                          WIFI_ATCommandTypeDef cmdType, ...) {
  // cmd to string
  int i = 0, bufferSize = MAX_COMMAND_LEN, result;
  char* cmd = wifi->command;
  const char* cmdString = WIFI_CommandString[cmdType];
  result = appendString(cmd, &bufferSize, &S(cmdString));
  wifi->commandLen = strlen(wifi->command);
  if (result != ARG_SUCCESS) {
    wifi->status = WIFI_STATUS_ERROR;
    return;
  }

  // args to string
  va_list args;
  va_start(args, cmdType);

  Arg a = va_arg(args, Arg);
  if (a.kind != ARG_END) {
    // '=' 삽입
    result = appendString(wifi->command, &bufferSize, &S('='));

    while (1) {
      char* cmd = wifi->command + wifi->commandLen;
      // 구분자 삽입
      if (i > 0) {
        if (appendString(cmd, &bufferSize, &S(',')) != ARG_SUCCESS) {
          break;
        }
      }
      i++;

      // 가변인자 파싱, 삽입
      if (a.kind == ARG_QS) {
        result = appendQuoteString(cmd, &bufferSize, &a);
      } else if (a.kind == ARG_S) {
        result = appendString(cmd, &bufferSize, &a);
      } else if (a.kind == ARG_I) {
        result = appendInt(cmd, &bufferSize, &a);
      }

      if (result != ARG_SUCCESS) {
        break;
      }
      wifi->commandLen = MAX_COMMAND_LEN - bufferSize;
      if (a.kind == ARG_END) {
        wifi->commandLen = strlen(wifi->command);
        break;
      }
    }
  }
  if (result != ARG_SUCCESS) {
    wifi->status = WIFI_STATUS_ERROR;
  }
  va_end(args);
}

void WIFI_SendCommand(WIFI_HandleTypeDef* wifi) {
  HAL_UART_Transmit_DMA(wifi->huart, wifi->command, wifi->commandLen);
  HAL_UARTEx_ReceiveToIdle_DMA(wifi->huart, wifi->response, MAX_COMMAND_LEN);
  wifi->lastTxTick = HAL_GetTick();
}

void WIFI_ProcessError(WIFI_HandleTypeDef* wifi) {}

HAL_StatusTypeDef WIFI_Tick(WIFI_HandleTypeDef* wifi) {
  if (wifi->status == WIFI_STATUS_ERROR) {
    WIFI_ProcessError(wifi);
  }

  if (wifi->status == WIFI_STATUS_BUSY) {
    if (wifi->lastTxTick + wifi->timeoutTickAmount < HAL_GetTick()) {
      // retry
      WIFI_SendCommand(wifi);
    }
  }
}

void WIFI_CommandSuccessCallback(WIFI_HandleTypeDef* wifi) {
  wifi->status = WIFI_STATUS_READY;
}

void WIFI_CommandErrorCallback(WIFI_HandleTypeDef* wifi) {
  wifi->status = WIFI_STATUS_ERROR;
}

void WIFI_ProcessRx(WIFI_HandleTypeDef* wifi) {
  if (strstr(wifi->response, BUSY_P) == NULL &&
      strstr(wifi->response, BUSY_S) == NULL) {
    WIFI_CommandSuccessCallback(wifi);
  } else {
    WIFI_CommandErrorCallback(wifi);
  }
}

HAL_StatusTypeDef WIFI_AT(WIFI_HandleTypeDef* wifi) {
  WIFI_GenerateCommand(wifi, AT, END);
  WIFI_SendCommand(wifi);
  wifi->status = WIFI_STATUS_BUSY;
  return HAL_OK;
}

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
