#include "variable_arg.h"

#include <string.h>

int appendQuoteString(char* dest, int* bufferSize, Arg* arg) {
  int result = appendString(dest, bufferSize, &S('"'));
  if (result != ARG_SUCCESS) {
    return result;
  }
  result = appendString(dest, bufferSize, arg);
  if (result != ARG_SUCCESS) {
    return result;
  }
  result = appendString(dest, bufferSize, &S('"'));
  if (result != ARG_SUCCESS) {
    return result;
  }
  return ARG_SUCCESS;
}

int appendString(char* dest, int* bufferSize, Arg* arg) {
  const char* str = arg->v.s;
  int len = strlen(str);
  if (*bufferSize < len) {
    return ARG_FAIL;
  }
  *bufferSize -= len;
  
  int i = 0;
  while (i < len) {
    dest[i] = str[i];
    i++;
  }
  return ARG_SUCCESS;
}

int appendInt(char* dest, int* bufferSize, Arg* arg) { return 0; }
