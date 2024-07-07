#ifndef _H_SESSION_

#define _H_SESSION_

#include <unistd.h>

/* result <- encoded char
 * m = URL_DECODE_M or URL_DECODE_L
 * */
#define URL_DECODE(result, p, m)                                               \
  if ('0' <= *(++p) && *p <= '9') {                                            \
    result += (*p - '0') << m;                                                 \
  } else {                                                                     \
    result += (*p - 'A' + 10) << m;                                            \
  }
#define URL_DECODE_M 4
#define URL_DECODE_L 0

#define RETURN_400(sock)                                                       \
  send(sock, HEADER_400 CRLF CRLF, HEADER_400_LEN + CRLF_LEN * 2, MSG_NOSIGNAL);

#define RETURN_500(sock)                                                       \
  send(sock, HEADER_500 CRLF CRLF, HEADER_500_LEN + CRLF_LEN * 2, MSG_NOSIGNAL);

#define FINISH_THREAD_NO_FREE(sock)                                            \
  close(sock);                                                                 \
  shutdown(sock, SHUT_RDWR);                                                   \
  return NULL

#define FINISH_THREAD(sock, buf, tag)                                          \
  free(buf);                                                                   \
  free(tag);                                                                   \
  close(sock);                                                                 \
  shutdown(sock, SHUT_RDWR);                                                   \
  return NULL

extern char *map_g;
extern off_t file_size_g;
extern char *map_end_g;
extern char **index_g;

void *session_thread(void *param);

#endif
