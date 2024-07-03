#ifndef _H_TAGTYPES_

#define _H_TAGTYPES_

#include <stdint.h>

typedef struct geotag_struct geotag_t;
struct geotag_struct {
  uint_fast32_t id;

  /* Date and time */
  uint_fast8_t year;
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
