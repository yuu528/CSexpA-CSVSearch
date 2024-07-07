#include "../config.h"
#include "./includes/csvloader.h"
#include "./includes/extras.h"
#include "./includes/session.h"
#include "./includes/socketutil.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Global variables */
char *map_g;
off_t file_size_g;
char *map_end_g;
char **index_g;

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

  /* Start server */
  uint_fast8_t sock_listen = tcp_listen(10028);

  /* Setup pthread attr */
  pthread_attr_t pth_attr;
  pthread_attr_init(&pth_attr);
  pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);

  /* Setup timeout */
  struct timeval tv;
  tv.tv_sec = TIMEOUT_SEC;
  tv.tv_usec = 0;

  while (1) {
    /* Create a new thread */
    pthread_t th;
    int *p_sock_client;

    p_sock_client = malloc(sizeof(int));
    *p_sock_client = accept(sock_listen, NULL, NULL);

    setsockopt(*p_sock_client, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
               sizeof(tv));

    if (*p_sock_client == -1) {
      perror(MSG_ERR_ACCEPT);
      free(p_sock_client);
      continue;
    }

    if (pthread_create(&th, &pth_attr, session_thread, p_sock_client) != 0) {
      perror(MSG_ERR_THREAD_CREATE);
      close(*p_sock_client);
      shutdown(*p_sock_client, SHUT_RDWR);
      free(p_sock_client);
    }
  }
}
