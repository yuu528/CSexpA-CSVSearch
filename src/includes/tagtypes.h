#ifndef _H_TAGTYPES_

#define _H_TAGTYPES_

#include <inttypes.h>
#include <stdint.h>

/* clang-format off */
#define URL_FORMAT "http://farm%" PRIuFAST8 ".static.flickr.com/%" PRIuFAST16 "/%" PRIuFAST32 "_%s.jpg"
#define DATE_FORMAT "%" PRIuFAST16 "-%02" PRIuFAST8 "-%02" PRIuFAST8 " %02" PRIuFAST8 ":%02" PRIuFAST8 ":%02" PRIuFAST8
/* clang-format on */

#define URL_ID2_LEN 10

typedef struct geotag_struct geotag_t;
struct geotag_struct {
  uint_fast32_t id;

  /* Date and time */
  uint_fast16_t year;
  uint_fast8_t month;
  uint_fast8_t day;

  uint_fast8_t hour;
  uint_fast8_t minute;
  uint_fast8_t second;

  /* Location */
  float lat;
  float lon;

  /* URL */
  uint_fast8_t server_id;
  uint_fast16_t url_id1;
  char *url_id2;

  /* Linked list */
  geotag_t *next;
};

typedef struct tag_struct tag_t;
struct tag_struct {
  char *tag;
  geotag_t *geotag; /* First geotag */
  uint_fast8_t geotag_count;

  /* Linked list */
  tag_t *next;
};

#endif
