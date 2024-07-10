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

void write_as_bin(char *filename, char *output) {
  FILE *fp = fopen(filename, "r");
  FILE *fp_out = fopen(output, "wb");

  if (fp == NULL || fp_out == NULL) {
    fprintf(stderr, MSG_ERR_FILE_OPEN);
    return;
  }

  char buf[FILE_BUFFER_SIZE];
  int len = 0;
  char *ptok, *pnext;
  char *pad = "\0";

  int lengths[6] = {MAX_TAG_LEN,
                    MAX_LAT_LON_LEN,
                    MAX_LAT_LON_LEN,
                    YEAR_LEN + MONTH_LEN + DAY_LEN + HOUR_LEN + MINUTE_LEN +
                        SECOND_LEN + SERVER_ID_LEN + MAX_URL_ID1_LEN,
                    MAX_ID_LEN,
                    URL_ID2_LEN};

  while (fgets(buf, FILE_BUFFER_SIZE, fp) != NULL) {
    /* Remove \n */
    buf[strlen(buf) - 1] = '\0';

    ptok = strtok_r(buf, CSV_DELIM, &pnext);

    /* Foreach with columns */
    for (int i = 0; i < 6; i++) {
      len = strlen(ptok);

      if (len > lengths[i]) {
        fprintf(stderr, MSG_ERR_LEN ": Column: %d, %d > %d, Field: %s\n", i,
                len, lengths[i], ptok);
        return;
      }

      fwrite(ptok, sizeof(char), len, fp_out);

      /* Pad with \0 */
      for (int j = len; j < lengths[i]; j++) {
        fwrite(pad, sizeof(char), 1, fp_out);
      }

      ptok = strtok_r(NULL, CSV_DELIM, &pnext);
    }
  }
}
