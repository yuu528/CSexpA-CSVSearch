#include "stringutil.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

uint_fast32_t atoi_uint_fast32(char *str) {
  int len = strlen(str);
  uint_fast32_t result = 0;

  for (int i = len - 1; i >= 0; i--) {
    if (str[i] < '0' || str[i] > '9') {
      return 0;
    }

    result += (str[i] - '0') * pow(10, len - i - 1);
  }

  return result;
}

uint_fast16_t atoi_uint_fast16(char *str) {
  int len = strlen(str);
  uint_fast16_t result = 0;

  for (int i = len - 1; i >= 0; i--) {
    if (str[i] < '0' || str[i] > '9') {
      return 0;
    }

    result += (str[i] - '0') * pow(10, len - i - 1);
  }

  return result;
}

uint_fast8_t atoi_uint_fast8(char *str) {
  int len = strlen(str);
  uint_fast8_t result = 0;

  for (int i = len - 1; i >= 0; i--) {
    if (str[i] < '0' || str[i] > '9') {
      return 0;
    }

    result += (str[i] - '0') * pow(10, len - i - 1);
  }

  return result;
}

uint_fast64_t hextoi_uint_fast64(char *str) {
  int len = strlen(str);
  uint_fast8_t result = 0;

  for (int i = len - 1; i >= 0; i--) {
    if ('0' <= str[i] && str[i] <= '9') {
      result += (str[i] - '0') * pow(16, len - i - 1);
    } else if ('a' <= str[i] && str[i] <= 'f') {
      result += (str[i] - 'a' + 10) * pow(16, len - i - 1);
    } else if ('A' <= str[i] && str[i] <= 'F') {
      result += (str[i] - 'A' + 10) * pow(16, len - i - 1);
    } else {
      return 0;
    }
  }

  return result;
}
