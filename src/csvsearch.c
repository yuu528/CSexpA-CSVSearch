#include "./includes/csvloader.h"
#include "./includes/socketutil.h"
#include "./includes/tagtypes.h"
#include "../config.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/socket.h>

#define TH_PARAM_SOCK *(int *)param

/* Global variables */
tag_t *tag_db_g[MAX_TAG_LEN];

void *thread_func(void *param) {
  char buf[RECV_SEND_SIZE];
  char tag[MAX_TAG_LEN + 1];

  /* Start session */
  recv(TH_PARAM_SOCK, buf, RECV_SEND_SIZE, 0);

  /* Find = */
  uint_fast16_t i = 0;
  while (buf[++i] != '=')
    ;

  /* Get tag */
  int_fast16_t j = -1;
  while (buf[++i] != ' ') {
    tag[++j] = buf[i];
  }
  tag[j + 1] = '\0';

  /* Start reply */
  /* clang-format off */
  uint_fast16_t len = sprintf(
    buf,
    HEADER_200 CRLF
    HEADER_CONTENT_TYPE MIME_JSON CRLF CRLF
    "{" JSON_KEY_TAG ":\"%s\"," JSON_KEY_RESULTS ":[",
    tag
  );
  /* clang-format on */
  send(TH_PARAM_SOCK, buf, len, MSG_NOSIGNAL);

  /* Search tag */
  uint_fast8_t tag_len = strlen(tag);
  if (tag_db_g[tag_len] != NULL) {
    tag_t *ptag = tag_db_g[tag_len];
    char result_sep = ' ';

    while (ptag != NULL) {
      if (strcmp(ptag->tag, tag) == 0) {
        /* Print geotags */
        geotag_t *geotag = ptag->geotag;
        do {
          /* clang-format off */
          len = sprintf(
            buf,
            "%c{" JSON_KEY_LAT ":%f,"
            JSON_KEY_LON ":%f,"
            JSON_KEY_DATE ":\"" DATE_FORMAT "\","
            JSON_KEY_URL ":\"" URL_FORMAT "\"}",
            result_sep,
            geotag->lat,
            geotag->lon,
            geotag->year, geotag->month, geotag->day, geotag->hour, geotag->minute, geotag->second,
            geotag->server_id, geotag->url_id1, geotag->id, geotag->url_id2
          );
          /* clang-format on */
          send(TH_PARAM_SOCK, buf, len, MSG_NOSIGNAL);
          result_sep = ',';
        } while ((geotag = geotag->next) != NULL);

        break;
      }

      ptag = ptag->next;
    }
  }

  /* Close json */
  /* clang-format off */
  send(
    TH_PARAM_SOCK,
    "]}" CRLF,
    2 + CRLF_LEN,
    MSG_NOSIGNAL
  );
  /* clang-format on */

  /* End session */
  close(TH_PARAM_SOCK);
  shutdown(TH_PARAM_SOCK, SHUT_RDWR);

  return NULL;
}

int main(int argc, char *argv[]) {
  /* Init on-memory database from csvs */
  if (argc < 2) {
    printf("Usage: %s <csv path>\n", argv[0]);
    printf("CSV file must be merged and sorted by date.\n");
    return 1;
  }

  /* Initialize tag_db_g */
  for (uint_fast8_t i = 0; i < MAX_TAG_LEN; i++) {
    tag_db_g[i] = NULL;
  }

  printf("Loading DB...\n");
  loadCSV(argv[1], tag_db_g);
  printf("\nDB loaded.\n");

  /* Debug: Print tag_db_g */
#ifdef OUTPUT_DB
  for (int i = 0; i < MAX_TAG_LEN; i++) {
    tag_t *tag = tag_db_g[i];
    while (tag != NULL) {
      printf("tag_db_g[%d] %" PRIuFAST8 ": %s\n", i, tag->geotag_count,
             tag->tag);

      geotag_t *geotag = tag->geotag;
      while (geotag != NULL) {
        /* clang-format off */
      printf(
        "  %" PRIuFAST32
        ", %" PRIuFAST32 "-%" PRIuFAST8 "-%" PRIuFAST8
        " %" PRIuFAST8 ":%" PRIuFAST8 ":%" PRIuFAST8
        ", %f, %f, "
        URL_FORMAT " \n",
        geotag->id,
        geotag->year, geotag->month, geotag->day,
        geotag->hour, geotag->minute, geotag->second,
        geotag->lat, geotag->lon,
        geotag->server_id, geotag->url_id1, geotag->id, geotag->url_id2
      );
        /* clang-format on */
        geotag = geotag->next;
      }

      tag = tag->next;
    }
  }
  printf("DB output finished.\n");
#endif

  /* Start server */
  uint_fast8_t sock_listen = tcp_listen(10028);

  /* Setup pthread attr */
  pthread_attr_t pth_attr;
  pthread_attr_init(&pth_attr);
  pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);

  while (1) {
    struct sockaddr addr;
    socklen_t len;
    int sock_client = accept(sock_listen, &addr, &len);

    /* Create a new thread */
    pthread_t th;
    pthread_create(&th, &pth_attr, thread_func, &sock_client);
  }
}
