#include "extras.h"
#include "../../config.h"

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
