#include "extras.h"

#include "../../config.h"
#include "tagtypes.h"

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
  char tag[MAX_TAG_LEN] = "";
  char *ptok, *pnext, *year, *month, *day, *hour, *minute, *second, *server_id;
  uint_fast16_t len = 0;
  long pos_ret = 0;
  long pos_tmp = 0;

  while (fgets(buf, FILE_BUFFER_SIZE, fp) != NULL) {
    ptok = strtok_r(buf, CSV_DELIM, &pnext);

    if (strcmp(tag, ptok) != 0) {
      if (len != 0) {
        len += fprintf(fp_out, "]}\n");

        /* Write len */
        pos_tmp = ftell(fp_out);
        fseek(fp_out, pos_ret, SEEK_SET);
        fwrite(&len, sizeof(uint_fast16_t), 1, fp_out);
        fseek(fp_out, pos_tmp, SEEK_SET);
      }

      strcpy(tag, ptok);

      /* Write tag */
      fprintf(fp_out, "%s,", tag);

      /* Keep space for len */
      pos_ret = ftell(fp_out);
      fseek(fp_out, sizeof(uint_fast16_t), SEEK_CUR);

      /* clang-format off */
      len = fprintf(
        fp_out,
        HEADER_200 CRLF
        HEADER_CONTENT_TYPE MIME_JSON CRLF CRLF
        "{" JSON_KEY_TAG ":\"%s\"," JSON_KEY_RESULTS ":[",
        tag
      );
      /* clang-format on */
    } else {
      len += fprintf(fp_out, ",");
    }

    /* Write results */
    /* clang-format off */
    len += fprintf(
      fp_out,
      "{"
      JSON_KEY_LAT ":%s,"
      JSON_KEY_LON ":%s,",
      strtok_r(NULL, CSV_DELIM, &pnext),
      strtok_r(NULL, CSV_DELIM, &pnext)
    );

    /* Write date */
    year = strtok_r(NULL, CSV_DELIM, &pnext);
    month = year + YEAR_LEN;
    day = month + MONTH_LEN;
    hour = day + DAY_LEN;
    minute = hour + HOUR_LEN;
    second = minute + MINUTE_LEN;
    server_id = second + SECOND_LEN;

    len += fprintf(
      fp_out,
      JSON_KEY_DATE ":\"" DATE_FORMAT_STR "\","
      JSON_KEY_URL ":\"http://farm%.1s.static.flickr.com/%s/%s_%s.jpg\"}",
      year, month, day, hour, minute, second,
      server_id, server_id + 1,
      strtok_r(NULL, CSV_DELIM, &pnext),
      pnext
    );
    /* clang-format on */
  }
}
