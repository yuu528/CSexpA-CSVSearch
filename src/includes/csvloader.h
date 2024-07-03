#ifndef _H_CSVLOADER_

#define _H_CSVLOADER_

#include "tagtypes.h"

#include <stdint.h>

// Maximum length of a line in the CSV file
#define FILE_BUFFER_SIZE 300

#define CSV_DELIM ","

#define MSG_ERR_MEM_ALLOC "Memory allocation error\n"

typedef enum { E_TAG_ID, E_TAG_NAME } tag_field_t;

uint_fast8_t loadTag(char *filename, tag_t *tag[]);

#endif
