#include "extras.h"
#include "../../config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void print_tag_length_csv(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, MSG_ERR_FILE_OPEN);
    return;
  }

  char buf[FILE_BUFFER_SIZE];
  char *ptok, *pnext;

  while (fgets(buf, FILE_BUFFER_SIZE, fp) != NULL) {
    ptok = strtok_r(buf, CSV_DELIM, &pnext);

    /* Check if tag is not empty */
    if (*pnext != ',') {
      /* Print id */
      printf("%s" CSV_DELIM, ptok);

      /* Print tag, length, remaining */
      ptok = strtok_r(NULL, CSV_DELIM, &pnext);
      printf("%s" CSV_DELIM "%zu" CSV_DELIM "%s", ptok, strlen(ptok), pnext);
    }
  }
}

void print_tag_limit_csv(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, MSG_ERR_FILE_OPEN);
    return;
  }

  char buf[FILE_BUFFER_SIZE], tag[MAX_TAG_LEN] = "";
  uint_fast8_t count = 0;
  char *ptok, *pnext;

  while (fgets(buf, FILE_BUFFER_SIZE, fp) != NULL) {
    ptok = strtok_r(buf, CSV_DELIM, &pnext);

    /* count */
    if (count >= MAX_GEOTAG_PER_TAG && strcmp(tag, ptok) == 0) {
      continue;
    } else if (strcmp(tag, ptok) != 0) {
      count = 0;
      strcpy(tag, ptok);
    }

    ++count;
    printf("%s" CSV_DELIM "%s", tag, pnext);
  }
}
