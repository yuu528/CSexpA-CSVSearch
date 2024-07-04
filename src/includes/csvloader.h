#ifndef _H_CSVLOADER_

#define _H_CSVLOADER_

#include "tagtypes.h"

#include <stdint.h>

#define DEBUG
/* More verbose debug */
// #define DEBUG_V
// #define DEBUG_VV

/* Maximum length of a line in the CSV file */
#define FILE_BUFFER_SIZE 512
#define MAX_GEOTAG_PER_TAG 100

#define CSV_DELIM ","
#define DATE_TIME_DELIM " "
#define DATE_DELIM "-"
#define TIME_DELIM ":"

#define MSG_ERR_FILE_OPEN "File open error\n"
#define MSG_ERR_MEM_ALLOC "Memory allocation error\n"

#define SKIP_STR_TO_NUMBER(p) while (*(++p) < '0' || '9' < *p)

typedef enum { E_TAG_ID, E_TAG_NAME } tag_field_t;

uint_fast8_t loadCSV(char *filename, tag_t *tag[]);
tag_t *create_new_tag(char *tagname, uint_fast8_t tag_len, char *id,
                      char *psave);
geotag_t *create_new_geotag(char *id, char *psave);
void parse_date(char *str, geotag_t *geotag);
void parse_url(char *str, geotag_t *geotag);

#endif
