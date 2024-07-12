#ifndef _H_CSVLOADER_

#define _H_CSVLOADER_

#include "../../config.h"
#include "tagtypes.h"

#include <stdint.h>
#include <sys/types.h>

#define GET_INDEX_KEY(len, char_addr)                                          \
  INDEX_SIZE_FACTOR *len + *((INDEX_TYPE *)char_addr)

long load_csv(char *filename, char **map, long *map_size);
char **create_index(char *map, off_t file_size);

#endif
