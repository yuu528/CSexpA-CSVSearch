#include "./includes/csvloader.h"
#include "./includes/socketutil.h"
#include "./includes/tagtypes.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/socket.h>

/* Config */
#define DEBUG

/* Thread parameter */
#define TH_PARAM_SOCK *(uint_fast8_t *)param

/* Constants */
#define MAX_TAG_LEN 255

#define RECV_SIZE 1024

#define HEADER_200 "HTTP/1.1 200 OK"
#define HEADER_200_LEN 15

#define HEADER_CONTENT_LENGTH "Content-Length: "
#define HEADER_CONTENT_LENGTH_LEN 16

#define HEADER_CONTENT_TYPE "Content-Type: "
#define HEADER_CONTENT_TYPE_LEN 14

#define CRLF "\r\n"
#define CRLF_LEN 2

#define MIME_JSON "application/json"
#define MIME_JSON_LEN 16

#define HEADER_LEN_DEFAULT                                                     \
  HEADER_200_LEN + HEADER_CONTENT_LENGTH_LEN + HEADER_CONTENT_TYPE_LEN +       \
      MIME_JSON_LEN + CRLF_LEN * 4

/* Global variables */
tag_t *tag_db_g[MAX_TAG_LEN];

void *thread_func(void *param) {
  uint_fast8_t buf[RECV_SIZE];

  /* Start session */
  recv(TH_PARAM_SOCK, buf, RECV_SIZE, 0);

  /* Finish reading */
  // shutdown(TH_PARAM_SOCK, SHUT_RD);

  /* Find = */
  uint_fast8_t i = 0;
  while (buf[++i] != '=')
    ;

  /* Get tag */
  uint_fast8_t *tag = &buf[i + 1];
  i = 0;
  while (tag[++i] != ' ')
    ;
  tag[i] = '\0';

  /* Reply */
  /* clang-format off */
  send(
    TH_PARAM_SOCK,
    HEADER_200 CRLF
    HEADER_CONTENT_LENGTH "2" CRLF
    HEADER_CONTENT_TYPE MIME_JSON CRLF CRLF
    "{}",
    HEADER_LEN_DEFAULT + 3,
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

  loadCSV(argv[1], tag_db_g);

  /* Debug: Print tag_db_g */
#ifdef DEBUG
  printf("DB loaded.\n");
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
    uint_fast8_t len;
    uint_fast8_t sock_client = accept(sock_listen, &addr, (socklen_t *)&len);

    /* Create a new thread */
    pthread_t th;
    pthread_create(&th, &pth_attr, thread_func, &sock_client);
  }
}
