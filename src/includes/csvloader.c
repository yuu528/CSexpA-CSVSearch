#include "csvloader.h"
#include "stringutil.h"
#include "tagtypes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint_fast8_t loadTag(char *filename, tag_t *tag_db[]) {
  FILE *fp;
  char buf[FILE_BUFFER_SIZE];
  char *ptok, *psave;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    return 1;
  }

  while (fgets(buf, FILE_BUFFER_SIZE, fp) != NULL) {
    uint_fast8_t tag_len;
    tag_field_t field = E_TAG_ID;
    geotag_t *new_geotag = malloc(sizeof(geotag_t));

    if (new_geotag == NULL) {
      fprintf(stderr, MSG_ERR_MEM_ALLOC);
      exit(1);
    }

    /* Remove \n */
    buf[strlen(buf) - 1] = '\0';

    ptok = strtok_r(buf, CSV_DELIM, &psave);
    while (ptok != NULL) {
      switch (field) {
      case E_TAG_ID:
        /* Read tag ID field */
        new_geotag->id = atoi_uint_fast32(ptok);
        field = E_TAG_NAME;
        break;

      case E_TAG_NAME:
        /* Read tag name field */
        tag_len = strlen(ptok);

        if (tag_db[tag_len] == NULL) {
          /* When it is a new tag */
          tag_db[tag_len] = malloc(sizeof(tag_t));
          if (tag_db[tag_len] == NULL) {
            fprintf(stderr, MSG_ERR_MEM_ALLOC);
            exit(1);
          }

          tag_db[tag_len]->tag = malloc(tag_len + 1);
          strcpy(tag_db[tag_len]->tag, ptok);

          tag_db[tag_len]->geotag = new_geotag;
        } else {
          /* When it is an existing tag */
          /* Find the last geotag */
          geotag_t *geotag = tag_db[tag_len]->geotag;
          while (geotag->next != NULL) {
            geotag = geotag->next;
          }

          geotag->next = new_geotag;
        }
        break;
      }

      ptok = strtok_r(NULL, CSV_DELIM, &psave);
    }
  }

  return 0;
}
