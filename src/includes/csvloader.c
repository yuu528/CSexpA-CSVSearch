#include "csvloader.h"
#include "stringutil.h"
#include "tagtypes.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint_fast8_t loadCSV(char *filename, tag_t *tag_db[]) {
  FILE *fp;
  char buf[FILE_BUFFER_SIZE];
  char *p_id, *p_tag, *psave;
  uint_fast8_t tag_len;

#ifdef DEBUG
  unsigned long line = 0;
#endif

  fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, MSG_ERR_FILE_OPEN);
    exit(1);
  }

  while (fgets(buf, FILE_BUFFER_SIZE, fp) != NULL) {
#ifdef DEBUG
    printf("\rline: %lu", ++line);
#endif

    p_id = strtok_r(buf, CSV_DELIM, &psave);

    if (*psave == ',') {
      /* Skip empty tag */
      continue;
    }
    p_tag = strtok_r(NULL, CSV_DELIM, &psave);

    tag_len = strlen(p_tag);
    if (tag_db[tag_len] == NULL) {

#ifdef DEBUG_V
      printf("\nCreate new tag\n");
#endif

      tag_db[tag_len] = create_new_tag(p_tag, tag_len, p_id, psave);

#ifdef DEBUG_V
      printf("tagname: %s\n", tag_db[tag_len]->tag);
#endif
    } else {
#ifdef DEBUG_V
      printf("\nExisting tag\n");
#endif
      /* Exising tag length */
      if (tag_db[tag_len]->geotag_count < MAX_GEOTAG_PER_TAG) {
        /* skip to last tag or same tag */
        tag_t *ptag = tag_db[tag_len];
        while (1) {
          if (strcmp(ptag->tag, p_tag) == 0) {
            /* Existing tag */
            /* Skip to last geotag */
            geotag_t *pgeotag = ptag->geotag;
            while (pgeotag->next != NULL) {
              pgeotag = pgeotag->next;
            }

            /* Create new geotag */
            pgeotag->next = create_new_geotag(p_id, psave);
            ++ptag->geotag_count;
            break;
          } else if (ptag->next != NULL) {
            ptag = ptag->next;
          } else {
            /* New tag */
            ptag->next = create_new_tag(p_tag, tag_len, p_id, psave);
            break;
          }
        }
      }
    }
  }

  return 0;
}

tag_t *create_new_tag(char *tagname, uint_fast8_t tag_len, char *id,
                      char *psave) {
  /* Create new tag */
  tag_t *tag = (tag_t *)malloc(sizeof(tag_t));
  if (tag == NULL) {
    fprintf(stderr, MSG_ERR_MEM_ALLOC);
    exit(1);
  }

  /* Set tagname */
  tag->tag = (char *)malloc((tag_len + 1) * sizeof(char));
  if (tag->tag == NULL) {
    fprintf(stderr, MSG_ERR_MEM_ALLOC);
    exit(1);
  }
  strncpy(tag->tag, tagname, tag_len);

#ifdef DEBUG_V
  printf("tagname: %s\n", tag->tag);
#endif

  tag->geotag = create_new_geotag(id, psave);
  tag->geotag_count = 1;

  return tag;
}

geotag_t *create_new_geotag(char *id, char *psave) {
  /* Create new geotag */
  geotag_t *geotag = (geotag_t *)malloc(sizeof(geotag_t));
  if (geotag == NULL) {
    fprintf(stderr, MSG_ERR_MEM_ALLOC);
    exit(1);
  }

  geotag->id = atoi_uint_fast32(id);
  parse_date(strtok_r(NULL, CSV_DELIM, &psave), geotag);
  geotag->lat = atof(strtok_r(NULL, CSV_DELIM, &psave));
  geotag->lon = atof(strtok_r(NULL, CSV_DELIM, &psave));
  parse_url(strtok_r(NULL, CSV_DELIM, &psave), geotag);

#ifdef DEBUG_VV
  printf("id: %" PRIuFAST32 "\n", geotag->id);
  printf("lat, lon: %f, %f\n", geotag->lat, geotag->lon);
#endif

  return geotag;
}

void parse_date(char *str, geotag_t *geotag) {
  char *p = str + 1; /* Skip first quote */
  char *ptok, *psave, *psave2;

  /* Parse date */
  ptok = strtok_r(p, DATE_TIME_DELIM, &psave);
  geotag->year = atoi_uint_fast16(strtok_r(ptok, DATE_DELIM, &psave2));
  geotag->month = atoi_uint_fast8(strtok_r(NULL, DATE_DELIM, &psave2));
  geotag->day = atoi_uint_fast8(strtok_r(NULL, DATE_DELIM, &psave2));

#ifdef DEBUG_VV
  printf("year, month, day: %" PRIuFAST16 ", %" PRIuFAST8 ", %" PRIuFAST8 "\n",
         geotag->year, geotag->month, geotag->day);
#endif

  /* Parse time */
  ptok = strtok_r(NULL, " ", &psave);
  ptok[strlen(ptok) - 1] = '\0'; /* Remove last quote */
  geotag->hour = atoi_uint_fast8(strtok_r(ptok, TIME_DELIM, &psave2));
  geotag->minute = atoi_uint_fast8(strtok_r(NULL, TIME_DELIM, &psave2));
  geotag->second = atoi_uint_fast8(strtok_r(NULL, TIME_DELIM, &psave2));

#ifdef DEBUG_VV
  printf("hour, minute, second: %" PRIuFAST8 ", %" PRIuFAST8 ", %" PRIuFAST8
         "\n",
         geotag->hour, geotag->minute, geotag->second);
#endif
}

void parse_url(char *str, geotag_t *geotag) {
  char *p = str, *p_slash;

  /* parse server id */
  /* skip to first number of the url */
  SKIP_STR_TO_NUMBER(p);
  geotag->server_id = *p - '0';

#ifdef DEBUG_VV
  printf("server_id: %" PRIuFAST8 "\n", geotag->server_id);
#endif

  /* parse url_id1 */
  SKIP_STR_TO_NUMBER(p);
  /* temporary replace '/' to '\0' */
  p_slash = strchr(p, '/');
  *p_slash = '\0';
  geotag->url_id1 = atoi_uint_fast32(p);
  /* restore '/' */
  *p_slash = '/';

#ifdef DEBUG_VV
  printf("url_id1: %" PRIuFAST32 "\n", geotag->url_id1);
#endif

  /* parse url_id2 */
  /* skip to next underscore */
  while (*(++p) != '_')
    ;
  ++p;
  /* replace '.' to '\0' */
  *(strchr(p, '.')) = '\0';
  /* to store id as 64bit uint */
  /*
  geotag->url_id2 = hextoi_uint_fast64(p);
  */
  /* store as char */
  geotag->url_id2 = (char *)malloc((URL_ID2_LEN + 1) * sizeof(char));
  if (geotag->url_id2 == NULL) {
    fprintf(stderr, MSG_ERR_MEM_ALLOC);
    exit(1);
  }
  strncpy(geotag->url_id2, p, URL_ID2_LEN);

#ifdef DEBUG_VV
  printf("url_id2: %" PRIuFAST64 "\n", geotag->url_id2);
#endif
}
