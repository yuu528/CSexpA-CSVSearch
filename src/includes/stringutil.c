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
