#include "csvloader.h"
#include "../../config.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

off_t load_csv(char *filename, char **map, long *map_size) {
  struct stat st;
  off_t file_size;
  int fd = open(filename, O_RDONLY);

  if (fd < 0) {
    perror(MSG_ERR_FILE_OPEN);
    exit(1);
  }

  /* Get file size */
  if (stat(filename, &st) != 0) {
    perror(MSG_ERR_FILE_STAT);
    exit(1);
  }
  file_size = st.st_size;

  *map = (char *)malloc(file_size + 1);
  read(fd, *map, file_size);
  *(*map + file_size) = '\0';

  close(fd);

  return file_size;
}

char **create_index(char *map, off_t file_size) {
  char **index = malloc(INDEX_SIZE * sizeof(char *));

  if (index == NULL) {
    perror(MSG_ERR_MEM_ALLOC);
    exit(1);
  }

  /* Initialize */
  for (uint_fast16_t i = 0; i < INDEX_SIZE; i++) {
    index[i] = NULL;
  }

  char *p = map;
  char *end = map + file_size - 1;
  char *p_last;
  uint_fast16_t tag_len = 0;
  uint_fast16_t reply_len = 0;

  int alt_index;

  while (p < end && tag_len < MAX_TAG_LEN) {
    /* Count tag length */
    tag_len = 0;
    while (*(p + (++tag_len)) != ',')
      ;

    /* Store tag index */
    alt_index = GET_INDEX_KEY(tag_len, p);
    if (index[alt_index] == NULL) {
      index[alt_index] = p;
    }
    p_last = p;

    /* Skip to next line */
#ifdef USE_BINARY
    p += tag_len + 1;
    reply_len = *((uint_fast16_t *)p);
    p += sizeof(uint_fast16_t) + reply_len;
#else
    while (*(++p) != '\n')
      ;
    ++p;
#endif
  }

  /* Pad index */
  for (int i = INDEX_SIZE - 1; i >= 0; --i) {
    if (index[i] == NULL) {
      index[i] = p_last;
    } else {
      p_last = index[i];
    }
  }

  return index;
}
