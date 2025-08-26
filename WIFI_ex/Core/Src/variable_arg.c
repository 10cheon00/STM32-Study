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

int appendInt(char* dest, int* bufferSize, Arg* arg) {
  int n = arg->v.i, i = 0;
  int len = 0, copy = n, rev = 0;
  while (copy > 0) {
    rev *= 10;
    rev = copy % 10;
    copy /= 10;
    len++;
  }
  if (*bufferSize < len) {
    return ARG_FAIL;
  }
  *bufferSize -= len;
  while (rev > 0) {
    dest[i++] = (rev % 10) + '0';
    rev /= 10;
  }
  return ARG_SUCCESS;
}
