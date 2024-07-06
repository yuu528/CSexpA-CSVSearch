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
  int page_size, fd = open(filename, O_RDONLY);

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

  /* Calc map size */
  page_size = getpagesize();
  *map_size = (file_size / page_size + 1) * page_size;

  /* Map file */
  *map = (char *)mmap(NULL, *map_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (*map == MAP_FAILED) {
    perror(MSG_ERR_MMAP);
    exit(1);
  }

  close(fd);

  return file_size;
}

char **create_index(char *map, off_t file_size) {
  char **index = malloc(sizeof(char *) * MAX_TAG_LEN);
  if (index == NULL) {
    perror(MSG_ERR_MEM_ALLOC);
    exit(1);
  }

  /* Initialize */
  for (uint_fast16_t i = 0; i < MAX_TAG_LEN; i++) {
    index[i] = NULL;
  }

  char *p = map;
  char *end = map + file_size - 1;
  uint_fast8_t tag_len = 0;
  while (p < end && tag_len < MAX_TAG_LEN) {
    /* Count tag length */
    tag_len = 0;
    while (*(p + (++tag_len)) != ',')
      ;

    /* Store tag index */
    if (index[tag_len] == NULL) {
      index[tag_len] = p;
    }

    p += tag_len;

    /* Skip to next line */
    while (*(++p) != '\n')
      ;
    ++p;
  }

  return index;
}
