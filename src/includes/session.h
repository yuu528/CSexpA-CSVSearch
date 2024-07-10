#ifndef _H_SESSION_

#define _H_SESSION_

#include "../../config.h"
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>

#ifdef SEND_NONBLOCK
#define SEND_FLAGS MSG_NOSIGNAL | MSG_DONTWAIT
#else
#define SEND_FLAGS MSG_NOSIGNAL
#endif

/* result <- encoded char
 * m = URL_DECODE_M or URL_DECODE_L
 * */

#ifdef ALT_URL_DECODE

#define URL_DECODE(result, p)                                                  \
  result = hex_table_g[*((uint16_t *)(++p)) - offset_g];                       \
  ++p;

#else

#define URL_DECODE(result, p, m) result += hex_table_g[(int)*(++p)] << m;
#define URL_DECODE_M 4
#define URL_DECODE_L 0

#endif

#define FINISH_THREAD(sock)                                                    \
  close(sock);                                                                 \
  shutdown(sock, SHUT_RDWR);                                                   \
  return

#ifdef CHECK_SEND_ERROR
#ifdef SEND_NONBLOCK
#define TRY_SEND(sock, buf, len, flags)                                        \
  while (send(sock, buf, len, flags) < 0) {                                    \
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)             \
      continue;                                                                \
    FINISH_THREAD(sock);                                                       \
  }
#else /* SEND_NONBLOCK */
#define TRY_SEND(sock, buf, len, flags)                                        \
  while (send(sock, buf, len, flags) < 0) {                                    \
    if (errno == EINTR)                                                        \
      continue;                                                                \
    FINISH_THREAD(sock);                                                       \
  }
#endif /* SEND_NONBLOCK */
#else  /* CHECK_SEND_ERROR */
#define TRY_SEND(sock, buf, len, flags) send(sock, buf, len, flags)
#endif /* CHECK_SEND_ERROR */

#define RETURN_400(sock)                                                       \
  TRY_SEND(sock, HEADER_400 CRLF CRLF, HEADER_400_LEN + CRLF_LEN * 2,          \
           SEND_FLAGS);

#define RETURN_500(sock)                                                       \
  TRY_SEND(sock, HEADER_500 CRLF CRLF, HEADER_500_LEN + CRLF_LEN * 2,          \
           SEND_FLAGS);

extern char *map_g;
extern off_t file_size_g;
extern char *map_end_g;
extern char **index_g;

#ifdef ALT_URL_DECODE
extern uint_fast8_t *hex_table_g;
extern uint_fast16_t offset_g;
#else
extern char hex_table_g[256];
#endif

void *session_thread(void *param);

#endif
