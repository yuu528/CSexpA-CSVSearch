#ifndef _H_CSVLOADER_

#define _H_CSVLOADER_

#include "tagtypes.h"

#include <stdint.h>
#include <sys/types.h>

long load_csv(char *filename, char **map, long *map_size);
char **create_index(char *map, off_t file_size);

#endif
