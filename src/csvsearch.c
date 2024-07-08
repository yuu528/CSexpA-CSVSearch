#include "../config.h"
#include "./includes/csvloader.h"
#include "./includes/extras.h"
#include "./includes/session.h"
#include "./includes/socketutil.h"

#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

/* Global variables */
char *map_g;
off_t file_size_g;
char *map_end_g;
char **index_g;

#ifdef ALT_URL_DECODE
uint_fast8_t *hex_table_g;
uint_fast16_t offset_g;
#else
char hex_table_g[256] = {0};
#endif

int main(int argc, char *argv[]) {
  /* Parse args */
  if (argc < 2) {
    printf("Usage: %s <option> <csv path>\n", argv[0]);
    printf("Options:\n");
    printf("  -c: Print tag length and exit.\n");
    printf("  -m: Print tags with 100 records per tag limit\n\n");
    printf("CSV file must be pre-processed by convert_csv.sh.\n");
    return 1;
  }

  /* Parse options */
  if (strcmp(argv[1], "-c") == 0) {
    if (argc < 3) {
      printf("Usage: %s -c <csv path>\n", argv[0]);
      return 1;
    }

    print_tag_length_csv(argv[2]);
    return 0;
  } else if (strcmp(argv[1], "-m") == 0) {
    if (argc < 3) {
      printf("Usage: %s -c <csv path>\n", argv[0]);
      return 1;
    }

    print_tag_limit_csv(argv[2]);
    return 0;
  }

  /* Load CSV to memory */
  long map_size;
  printf(MSG_INFO_LOADING "\n");
  file_size_g = load_csv(argv[1], &map_g, &map_size);
  printf(MSG_INFO_DONE "\n");

  /* Create index */
  printf(MSG_INFO_CREATE_INDEX "\n");
  index_g = create_index(map_g, file_size_g);
  printf(MSG_INFO_DONE "\n");

  map_end_g = map_g + file_size_g - 1;

/* Setup hex table */
#ifndef ALT_URL_DECODE
  for (uint_fast16_t i = '0'; i <= '9'; ++i) {
    hex_table_g[i] = i - '0';
#else
  char tmp[3] = "00";
  offset_g = *((uint16_t *)(tmp));
  tmp[0] = 'F';
  tmp[1] = 'F';
  int size = *((uint16_t *)(tmp)) - offset_g + 1;

  hex_table_g = (uint_fast8_t *)malloc(size * sizeof(uint_fast8_t));
  if (hex_table_g == NULL) {
    return 1;
#endif
  }
#ifndef ALT_URL_DECODE
  for (uint_fast16_t i = 'A'; i <= 'F'; ++i) {
    hex_table_g[i] = i - 'A' + 10;
#else
  for (uint_fast16_t i = '0'; i <= '9'; i++) {
    for (uint_fast16_t j = '0'; j <= '9'; j++) {
      tmp[0] = i;
      tmp[1] = j;
      hex_table_g[*((uint16_t *)(tmp)) - offset_g] = (i - '0') << 4 | (j - '0');
    }

    for (uint_fast16_t j = 'A'; j <= 'F'; j++) {
      tmp[0] = i;
      tmp[1] = j;
      hex_table_g[*((uint16_t *)(tmp)) - offset_g] =
          (i - '0') << 4 | (j - 'A' + 10);
    }
  }

  for (uint_fast16_t i = 'A'; i <= 'F'; i++) {
    for (uint_fast16_t j = '0'; j <= '9'; j++) {
      tmp[0] = i;
      tmp[1] = j;
      hex_table_g[*((uint16_t *)(tmp)) - offset_g] =
          (i - 'A' + 10) << 4 | (j - '0');
    }

    for (uint_fast16_t j = 'A'; j <= 'F'; j++) {
      tmp[0] = i;
      tmp[1] = j;
      hex_table_g[*((uint16_t *)(tmp)) - offset_g] =
          (i - 'A' + 10) << 4 | (j - 'A' + 10);
    }
#endif
  }

  /* Start server */
  uint_fast8_t sock_listen = tcp_listen(10028);

  /* Setup pthread attr */
  pthread_attr_t pth_attr;
  pthread_attr_init(&pth_attr);
  pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);

  /* Setup socket options */
  struct timeval tv;
  tv.tv_sec = TIMEOUT_SEC;
  tv.tv_usec = 0;

  int optval_true = 1;

  struct linger linger;
  linger.l_onoff = 0;

  /* Setup epoll */
#ifdef USE_EPOLL
  int event_count;
  struct epoll_event ev;
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror(MSG_ERR_EPOLL_CREATE);
    exit(1);
  }

  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = EPOLLIN;
  ev.data.fd = sock_listen;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_listen, &ev);
#endif

  while (1) {
    /* Create a new thread */
    pthread_t th;
    int *p_sock_client;

#ifdef USE_EPOLL
    event_count = epoll_wait(epoll_fd, &ev, 1, -1);
#ifdef CHECK_EPOLL_ERROR
    if (event_count == -1) {
      perror(MSG_ERR_EPOLL_WAIT);
      exit(1);
    }
#endif

    if (ev.data.fd != sock_listen) {
      continue;
    }
#endif

    p_sock_client = malloc(sizeof(int));

#ifdef CHECK_ACCEPT_ERROR
    if ((
#endif
                *p_sock_client = accept(sock_listen, NULL, NULL)
#ifdef CHECK_ACCEPT_ERROR
                    ) < 0) {
      perror(MSG_ERR_ACCEPT);
      free(p_sock_client);
      continue;
    }
#else
        ;
#endif

#ifdef ENABLE_TIMEOUT
    setsockopt(*p_sock_client, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
               sizeof(tv));
    setsockopt(*p_sock_client, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,
               sizeof(tv));
#endif
#ifdef ENABLE_TCP_NODELAY
    setsockopt(*p_sock_client, IPPROTO_TCP, TCP_NODELAY, &optval_true, 1);
#endif
#ifdef ENABLE_TCP_CORK
    setsockopt(*p_sock_client, IPPROTO_TCP, TCP_CORK, &optval_true, 1);
#endif
#ifdef DISABLE_LINGER
    setsockopt(*p_sock_client, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
#endif

#ifdef CHECK_THREAD_CREATE_ERROR
    if (
#endif
        pthread_create(&th, &pth_attr, session_thread, p_sock_client)
#ifdef CHECK_THREAD_CREATE_ERROR
        != 0) {
      perror(MSG_ERR_THREAD_CREATE);
      close(*p_sock_client);
      shutdown(*p_sock_client, SHUT_RDWR);
      free(p_sock_client);
    }
#else
        ;
#endif
  }
}
