#ifndef _H_CSVLOADER_

#define _H_CSVLOADER_

#include "tagtypes.h"

#include <stdint.h>

#define SKIP_STR_TO_NUMBER(p) while (*(++p) < '0' || '9' < *p)

typedef enum { E_TAG_ID, E_TAG_NAME } tag_field_t;

uint_fast8_t load_csv(char *filename, tag_t *tag[]);
tag_t *create_new_tag(char *tagname, uint_fast8_t tag_len, char *id,
                      char *psave);
geotag_t *create_new_geotag(char *id, char *psave);
void parse_date(char *str, geotag_t *geotag);
void parse_url(char *str, geotag_t *geotag);

#endif
