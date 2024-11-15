#include "session.h"

#include "../../config.h"
#include "csvloader.h"
#include "tagtypes.h"

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static inline __attribute__((always_inline)) void session(int sock) {
#ifdef MEASURE_TIME
  struct timespec ts_start;
  clock_gettime(CLOCK_MONOTONIC, &ts_start);
#endif

#ifdef USE_LARGE_BUFFER
  char buf[RECV_SEND_SIZE_LARGE];
  char *p_buf = buf;
#else
  char buf[RECV_SEND_SIZE];
#endif

  char tag[MAX_TAG_LEN];

#ifndef DISABLE_ESCAPE
  char tag_esc[MAX_TAG_LEN_ESCAPED];
  char *p_tag_esc = tag_esc - 1;
#endif

/* Start session */
#ifdef CHECK_RECV_LENGTH
  if (
#endif
      recv(sock, buf, RECV_SEND_SIZE, 0)
#ifdef CHECK_RECV_LENGTH
      <= SKIP_HEADER_FIRST_LEN) {
    RETURN_500(sock);
    FINISH_THREAD(sock);
  }
#else
      ;
#endif

  /* Check if it is a valid request */
  /*
   * GET /?tag=... HTTP/1.1
   *          ^ *p
   */
  char *p = buf + SKIP_HEADER_FIRST_LEN;
#ifdef CHECK_QUERY_EQUAL
  if (*p != '=') {
    RETURN_400(sock);
    FINISH_THREAD(sock);
  }
#endif

  /* Get tag */
  char *ptag = tag - 1;
  while (*(++p) != ' ') {
#ifndef DISABLE_URL_DECODE
    /* url decode */
    if (*p == '%') {
      URL_DECODE(*(++ptag), p);
    } else {
#endif /* DISABLE_URL_DECODE */
      *(++ptag) = *p;
#ifndef DISABLE_URL_DECODE
    }
#endif /* DISABLE_URL_DECODE */

#ifndef DISABLE_ESCAPE
    /* escape " */
    if (*ptag == '"') {
      *(++p_tag_esc) = '\\';
    }

    *(++p_tag_esc) = *ptag;
#endif /* DISABLE_ESCAPE */
  }
  *(++ptag) = '\0';

#ifndef DISABLE_ESCAPE
  *(++p_tag_esc) = '\0';
#endif

  /* Start reply */
  /* Search tag */
  uint_fast16_t tag_len = ptag - tag;

#ifndef USE_BINARY
#ifdef USE_LARGE_BUFFER
  p_buf +=
#else
  uint_fast16_t len =
#endif
      /* clang-format off */
    sprintf(
      buf,
      HEADER_200 CRLF
      HEADER_CONTENT_TYPE MIME_JSON CRLF CRLF
      "{" JSON_KEY_TAG ":\"%s\"," JSON_KEY_RESULTS ":[",
#ifdef DISABLE_ESCAPE
      tag
#else
      tag_esc
#endif
    );
  /* clang-format on */
#ifndef USE_LARGE_BUFFER
  TRY_SEND(sock, buf, len, SEND_FLAGS | MSG_MORE);
#endif
#endif

  /* CSV: tag,lat,...
          ^ *p_db */
  /* Set index as next_tag_len to use for finding next index */
  int next_tag_len = GET_INDEX_KEY(tag_len, tag);

  char *p_db = index_g[next_tag_len++];
  char *p_input;

#ifdef QUICK_INDEX
  char *p_end = index_g[next_tag_len];
#else
  char *p_end;
#endif

#ifdef USE_BINARY
  uint_fast16_t reply_len;
#else
  char *lat, *lon, *year, *month, *day, *hour, *minute, *second, *server_id,
      *url_id1, *id;
  int lat_len, lon_len, url_id1_len, id_len;
  char result_sep = ' ';
  uint_fast8_t matched = 0;
#endif

#ifndef QUICK_INDEX
  while (1) {
    if (index_g[next_tag_len] != NULL) {
      p_end = index_g[next_tag_len];
      break;
    }

    if (++next_tag_len > MAX_TAG_LEN) {
      p_end = map_end_g;
      break;
    }
  }
#endif

  if (p_db != NULL) {
    do {
      /* Check tag */
      /* first
       *     tag,lat,...
       *     ^ *p_db
       *     tag
       *     ^ *p_input
       *
       * next
       *     tag,lat,...
       *      ^ *p_db
       *     tag
       *      ^ *p_input    Not matched if *p_db != *p_input
       *
       * ...
       * last
       *     tag,lat,...
       *        ^ *p_db
       *     tag
       *        ^ *p_input  Matched if *p_db == ',' && *p_input == '\0'
       */
      p_input = tag - 1;

      --p_db;
      while (*(++p_input) != '\0') {
        if (*(++p_db) != *p_input) {
          /* Not matched */
#ifndef USE_BINARY
          if (matched == 1) {
            goto finish;
          }
#endif

          // printf("no\n");
#ifdef USE_BINARY
          while (*(++p_db) != ',')
            ;
          reply_len = *((uint_fast16_t *)(++p_db));
          p_db += sizeof(uint_fast16_t);
#else
          p_db += SKIP_TO_NEXT_LINE_FIRST;
#endif
          goto to_next;
        }
      }

      /* Matched */
      // printf("yes\n");
#ifdef USE_BINARY
      p_db += 2;
      reply_len = *((uint_fast16_t *)p_db);

      p_db += sizeof(uint_fast16_t);
      TRY_SEND(sock, p_db, reply_len, SEND_FLAGS);
      FINISH_THREAD(sock);
#else
      matched = 1;
      /* Get lat */
      /* Current
       *     tag,lat,...
       *        ^ *p_db
       *
       * Skip to ','
       *     tag,lat,...
       *            ^ *p_db
       */
      lat = (p_db += 2);
      while (*(++p_db) != ',')
        ;
      lat_len = p_db - lat;

      /* Get lon */
      lon = ++p_db;
      while (*(++p_db) != ',')
        ;
      lon_len = p_db - lon;

      /* Get date */
      /* Current
       *     tag,lat,lon,YYYYMMDDHHMMSS,...
       *                ^ *p_db
       *
       * Year
       *                     v *p_db
       *     tag,lat,lon,YYYYMMDDHHMMSS,...
       *                 ^ *year
       * ...
       * Second
       *                               v *p_db
       *     tag,lat,lon,YYYYMMDDHHMMSS...
       *                             ^ *second
       */
      year = ++p_db;
      p_db += YEAR_LEN;
      month = p_db;
      p_db += MONTH_LEN;
      day = p_db;
      p_db += DAY_LEN;
      hour = p_db;
      p_db += HOUR_LEN;
      minute = p_db;
      p_db += MINUTE_LEN;
      second = p_db;
      p_db += SECOND_LEN;

      /* Get server_id */
      server_id = p_db;

      /* Get url_id1 */
      url_id1 = ++p_db;
      while (*(++p_db) != ',')
        ;
      url_id1_len = p_db - url_id1;

      /* Get id */
      id = ++p_db;
      while (*(++p_db) != ',')
        ;
      id_len = p_db - id;

      /* clang-format off */
#ifdef USE_LARGE_BUFFER
      p_buf +=
#else
      len =
#endif
        sprintf(
#ifdef USE_LARGE_BUFFER
          p_buf,
#else
          buf,
#endif
          "%c{"
          JSON_KEY_LAT ":%.*s,"
          JSON_KEY_LON ":%.*s,"
          JSON_KEY_DATE ":\"" DATE_FORMAT_STR "\","
          JSON_KEY_URL ":\"" URL_FORMAT_STR "\""
          "}",
          result_sep,
          lat_len, lat,
          lon_len, lon,
          year, month, day, hour, minute, second,
          server_id, url_id1_len, url_id1, id_len, id, ++p_db
        );
      /* clang-format on */

#ifndef USE_LARGE_BUFFER
      TRY_SEND(sock, buf, len, SEND_FLAGS | MSG_MORE);
#endif
      result_sep = ',';
#endif

    to_next:
#ifdef USE_BINARY
      p_db += reply_len;
#else
      /* Skip to next line */
      while (*(++p_db) != '\n')
        ;
      ++p_db;
#endif
    } while (p_db < p_end);
  }

#ifdef USE_BINARY
  /* Not found */
  reply_len = sprintf(buf,
                      HEADER_200 CRLF HEADER_CONTENT_TYPE MIME_JSON CRLF CRLF
                      "{" JSON_KEY_TAG ":\"%s\"," JSON_KEY_RESULTS ":[]}",
#ifdef DISABLE_ESCAPE
                      tag
#else
                      tag_esc
#endif
  );

  send(sock, buf, reply_len, SEND_FLAGS);
#else /* USE_BINARY */
  /* Close json */
finish:
#ifdef USE_LARGE_BUFFER
  *p_buf = ']';
  *(++p_buf) = '}';
  TRY_SEND(sock, buf, p_buf - buf + 1, SEND_FLAGS);
#else
  /* clang-format off */
  TRY_SEND(
    sock,
    "]}" CRLF,
    2 + CRLF_LEN,
    SEND_FLAGS
  );
  /* clang-format on */
#endif
#endif /* USE_BINARY */

  /* End session */
  FINISH_THREAD(sock);
}

void *session_thread(void *restrict param) {
  int sock = *(int *)param;

#ifdef PRE_THREAD
#ifdef ACCEPT_ON_CHILD
  /* Setup epoll */
  int event_count;
  int epoll_fd = epoll_create1(0);

  if (epoll_fd == -1) {
    perror(MSG_ERR_EPOLL_CREATE);
    exit(1);
  }

  struct epoll_event ev;

  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = sock;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

#if defined(ENABLE_TCP_NODELAY) || defined(ENABLE_TCP_CORK)
  int optval_true = 1;
#endif

#ifdef DISABLE_LINGER
  struct linger linger;
  linger.l_onoff = 0;
#endif

  int sock_client;

  /* Event loop */
  while (1) {
#ifdef CHECK_EPOLL_ERROR
    if (
#endif /* CHECK_EPOLL_ERROR */
        epoll_wait(epoll_fd, &ev, 1, -1)
#ifdef CHECK_EPOLL_ERROR
        == -1) {
      perror(MSG_ERR_EPOLL_WAIT);
      exit(1);
    }
#else  /* CHECK_EPOLL_ERROR */
        ;
#endif /* CHECK_EPOLL_ERROR */

    if (ev.data.fd != sock) {
      continue;
    }

#ifdef CHECK_ACCEPT_ERROR
    if ((
#endif
            sock_client = accept(sock, NULL, NULL)
#ifdef CHECK_ACCEPT_ERROR
                ) < 0) {
      perror(MSG_ERR_ACCEPT);
      continue;
    }
#else
        ;
#endif

#ifdef ENABLE_TIMEOUT
    setsockopt(sock_client, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(sock_client, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
#endif
#ifdef ENABLE_TCP_NODELAY
    setsockopt(sock_client, IPPROTO_TCP, TCP_NODELAY, &optval_true, 1);
#endif
#ifdef ENABLE_TCP_CORK
    setsockopt(sock_client, IPPROTO_TCP, TCP_CORK, &optval_true, 1);
#endif
#ifdef DISABLE_LINGER
    setsockopt(sock_client, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
#endif

    session(sock_client);
  }
#else /* ACCEPT_ON_CHILD */
  int client_sock;
  int next_head;

  while (1) {
    pthread_mutex_lock(&mutex_g);
    pthread_cond_wait(&cond_g, &mutex_g);

    /* Pop from queue */
    next_head = sock_queue_head_g + 1;
    if (next_head >= SOCK_QUEUE_SIZE) {
      next_head = 0;
    }

    client_sock = sock_queue_g[next_head];
    sock_queue_head_g = next_head;
    pthread_mutex_unlock(&mutex_g);

    session(client_sock);
  }
#endif
#else /* PRE_THREAD */
  free(param);
  session(sock);
#endif

  return NULL;
}
