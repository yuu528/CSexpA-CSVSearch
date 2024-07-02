#include "./includes/socketutil.h"

#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/socket.h>

#define TH_PARAM_SOCK *(int *)param

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

void *thread_func(void *param) {
  char buf[RECV_SIZE];

  /* Start session */
  recv(TH_PARAM_SOCK, buf, RECV_SIZE, 0);

  /* Find = */
  int i = 0;
  while (buf[++i] != '=')
    ;

  /* Get tag */
  char *tag = &buf[i + 1];
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

int main(void) {
  int sock_listen = tcp_listen(10028);

  /* Setup pthread attr */
  pthread_attr_t pth_attr;
  pthread_attr_init(&pth_attr);
  pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);

  while (1) {
    struct sockaddr addr;
    int len;
    int sock_client = accept(sock_listen, &addr, (socklen_t *)&len);

    /* Create a new thread */
    pthread_t th;
    pthread_create(&th, &pth_attr, thread_func, &sock_client);
  }
}
