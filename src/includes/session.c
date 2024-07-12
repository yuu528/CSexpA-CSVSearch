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
  *(++p_tag_esc) = '\0';

  /* Start reply */
  /* Search tag */
  uint_fast16_t tag_len = ptag - tag;

  /* CSV: tag,lat,...
          ^ *p_db */
  /* Set index as next_tag_len to use for finding next index */
  int next_tag_len = GET_INDEX_KEY(tag_len, *tag_esc);
  char *p_db = index_g[next_tag_len++];
  char *p_input;
  char *p_end;
  uint_fast16_t reply_len;

  /* find next index */
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
          while (*(++p_db) != ',')
            ;
          reply_len = *((uint_fast16_t *)(++p_db));
          p_db += sizeof(uint_fast16_t);
          goto to_next;
        }
      }

      /* Matched */
      p_db += 2;
      reply_len = *((uint_fast16_t *)p_db);

      p_db += sizeof(uint_fast16_t);
      TRY_SEND(sock, p_db, reply_len, SEND_FLAGS);
      FINISH_THREAD(sock);

    to_next:
      p_db += reply_len;
    } while (p_db < p_end);
  }

  /* Not found */
  reply_len = sprintf(buf,
                      HEADER_200 CRLF HEADER_CONTENT_TYPE MIME_JSON CRLF CRLF
                      "{" JSON_KEY_TAG ":\"%s\"," JSON_KEY_RESULTS ":[]}",
                      tag_esc);
  send(sock, buf, reply_len, SEND_FLAGS);

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
