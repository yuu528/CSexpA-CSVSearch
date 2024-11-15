#include "../config.h"
#include "./includes/csvloader.h"
#include "./includes/extras.h"
#include "./includes/session.h"
#include "./includes/socketutil.h"

#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
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

#ifndef ACCEPT_ON_CHILD
pthread_mutex_t mutex_g;
pthread_cond_t cond_g = PTHREAD_COND_INITIALIZER;
int sock_queue_g[SOCK_QUEUE_SIZE];
uint_fast16_t sock_queue_head_g = -1;
uint_fast16_t sock_queue_tail_g = -1;
#endif

uint_fast8_t *hex_table_g;
uint_fast16_t offset_g;

int main(int argc, char *argv[]) {
  /* Parse args */
  if (argc < 2) {
    printf("Usage: %s <option> <csv path>\n", argv[0]);
    printf("Options:\n");
    printf("  -c: Print tag length and exit.\n");
    printf("  -m: Print tags with 100 records per tag limit\n\n");
    printf("  -b <Output file>: Output csv as binary\n\n");
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
  } else if (strcmp(argv[1], "-b") == 0) {
    if (argc < 4) {
      printf("Usage: %s -b <output path> <csv file>\n", argv[0]);
      return 1;
    }

    write_as_bin(argv[3], argv[2]);
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
  char tmp[3] = "00";
  offset_g = *((uint16_t *)(tmp));
  tmp[0] = 'F';
  tmp[1] = 'F';
  int size = *((uint16_t *)(tmp)) - offset_g + 1;

  hex_table_g = (uint_fast8_t *)malloc(size * sizeof(uint_fast8_t));
  if (hex_table_g == NULL) {
    return 1;
  }
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
  }

  /* Start server */
  int sock_listen = tcp_listen(10028);

  /* Setup pthread attr */
  pthread_attr_t pth_attr;
  pthread_attr_init(&pth_attr);
  pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);

  /* Setup socket options */
  struct timeval tv;
  tv.tv_sec = TIMEOUT_SEC;
  tv.tv_usec = 0;

#if defined(ENABLE_TCP_NODELAY) || defined(ENABLE_TCP_CORK)
  int optval_true = 1;
#endif

#ifdef DISABLE_LINGER
  struct linger linger;
  linger.l_onoff = 0;
#endif

#ifdef PRE_THREAD
  int *p_sock;
  p_sock = malloc(sizeof(int));
  *p_sock = sock_listen;

#ifndef ACCEPT_ON_CHILD
  pthread_mutex_init(&mutex_g, NULL);
#endif /* ACCEPT_ON_CHILD */

  /* Create threads */
  pthread_t ths[PRE_THREAD_COUNT];

  for (uint_fast16_t i = 0; i < PRE_THREAD_COUNT; ++i) {
#ifdef CHECK_THREAD_CREATE_ERROR
    if (
#endif
#ifdef ACCEPT_ON_CHILD
        pthread_create(&ths[i], &pth_attr, session_thread, p_sock)
#else  /* ACCEPT_ON_CHILD */
    pthread_create(&ths[i], &pth_attr, session_thread, NULL)
#endif /* ACCEPT_ON_CHILD */
#ifdef CHECK_THREAD_CREATE_ERROR
        != 0) {
      perror(MSG_ERR_THREAD_CREATE);
      exit(1);
    }
#else
        ;
#endif
  }
#ifdef ACCEPT_ON_CHILD
  pthread_exit(0);
#else /* ACCEPT_ON_CHILD */
  struct pollfd fds[1];

  fds[0].fd = sock_listen;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  int sock_tmp, next_tail;

  while (1) {
    if (poll(fds, 1, -1) < 0) {
      perror(MSG_ERR_POLL);
      exit(1);
    }

    /* received */
    if (fds[0].revents & POLLIN) {

#ifdef CHECK_ACCEPT_ERROR
      if ((
#endif
              sock_tmp = accept(sock_listen, NULL, NULL)
#ifdef CHECK_ACCEPT_ERROR
                  ) < 0) {
        perror(MSG_ERR_ACCEPT);
        continue;
      }
#else
          ;
#endif

#ifdef ENABLE_TIMEOUT
      setsockopt(sock_tmp, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
      setsockopt(sock_tmp, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
#endif
#ifdef ENABLE_TCP_NODELAY
      setsockopt(sock_tmp, IPPROTO_TCP, TCP_NODELAY, &optval_true, 1);
#endif
#ifdef ENABLE_TCP_CORK
      setsockopt(sock_tmp, IPPROTO_TCP, TCP_CORK, &optval_true, 1);
#endif
#ifdef DISABLE_LINGER
      setsockopt(sock_tmp, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
#endif

      /* Add to queue */
      pthread_mutex_lock(&mutex_g);
      if (sock_queue_tail_g >= SOCK_QUEUE_SIZE - 1) {
        sock_queue_tail_g = -1;
      }

      next_tail = sock_queue_tail_g + 1;

      if (next_tail == sock_queue_head_g) {
        pthread_mutex_unlock(&mutex_g);
        continue;
      }

      sock_queue_g[next_tail] = sock_tmp;
      sock_queue_tail_g = next_tail;

      pthread_cond_signal(&cond_g);
      pthread_mutex_unlock(&mutex_g);
    }
  }
#endif /* ACCEPT_ON_CHILD */
#else  /* USE_THREAD_POOL */
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
#endif /* USE_THERAD_POOL */
}
