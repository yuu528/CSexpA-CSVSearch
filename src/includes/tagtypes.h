#ifndef _H_TAGTYPES_

#define _H_TAGTYPES_

#include "../../config.h"

#include <inttypes.h>

/* clang-format off */
#define URL_FORMAT "http://farm%" PRIuFAST8 ".static.flickr.com/%" PRIuFAST16 "/%" PRIuFAST32 "_%s.jpg"
#define DATE_FORMAT "%" PRIuFAST16 "-%02" PRIuFAST8 "-%02" PRIuFAST8 " %02" PRIuFAST8 ":%02" PRIuFAST8 ":%02" PRIuFAST8

#define GETSTR_IN(x) #x
#define GETSTR(x) GETSTR_IN(x)

#define URL_FORMAT_STR "http://farm%." GETSTR(SERVER_ID_LEN) "s.static.flickr.com/%.*s/%.*s_%." GETSTR(URL_ID2_LEN) "s.jpg"
#define DATE_FORMAT_STR "%." GETSTR(YEAR_LEN) "s-%." GETSTR(MONTH_LEN) "s-%." GETSTR(DAY_LEN) "s %." GETSTR(HOUR_LEN) "s:%." GETSTR(MINUTE_LEN) "s:%." GETSTR(SECOND_LEN) "s"
/* clang-format on */

#define URL_ID2_LEN 10

#endif
