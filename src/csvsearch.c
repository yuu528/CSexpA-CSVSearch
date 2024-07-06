#include "../config.h"
#include "./includes/csvloader.h"
#include "./includes/extras.h"
#include "./includes/socketutil.h"
#include "./includes/tagtypes.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define TH_PARAM_SOCK *(int *)param

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

#define FINISH_THREAD()                                                        \
  close(TH_PARAM_SOCK);                                                        \
  shutdown(TH_PARAM_SOCK, SHUT_RDWR);                                          \
  return NULL

/* Global variables */
char *map_g;
off_t file_size_g;
char *map_end_g;
char **index_g;

void *thread_func(void *param) {
  char buf[RECV_SEND_SIZE];
  char tag[MAX_TAG_LEN + 1];

  /* Start session */
  recv(TH_PARAM_SOCK, buf, RECV_SEND_SIZE, 0);

  /* Finish if buffer is empty */
  if (buf[0] == '\0') {
    FINISH_THREAD();
  }

  /* Replace first \r\n with \0 */
  char *p = buf;
  while (*(++p) != '\r')
    ;
  *p = '\0';

  /* Find = */
  p = buf;
  while (*(++p) != '=') {
    if (*p == '\0') {
      send(TH_PARAM_SOCK, HEADER_400 CRLF CRLF, HEADER_400_LEN + CRLF_LEN * 2,
           MSG_NOSIGNAL);
      FINISH_THREAD();
    }
  }

  /* Get tag */
  char *ptag = tag - 1;
  while (*(++p) != ' ') {
    /* url decode */
    if (*p == '%') {
      *(++ptag) = 0;
      URL_DECODE(*ptag, p, URL_DECODE_M);
      URL_DECODE(*ptag, p, URL_DECODE_L);
    } else {
      *(++ptag) = *p;
    }
  }
  *(++ptag) = '\0';

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
  uint_fast16_t tag_len = strlen(tag);

  /* CSV: tag,lat,...
          ^ *p_db */
  char *p_db = index_g[tag_len];
  char *p_input;
  char *p_end;
  char *lat, *lon, *year, *month, *day, *hour, *minute, *second, *server_id,
      *url_id1, *id;
  int lat_len, lon_len, url_id1_len, id_len;
  char result_sep = ' ';

  /* find next index */
  uint_fast16_t next_tag_len = tag_len + 1;
  while (next_tag_len <= MAX_TAG_LEN) {
    if (index_g[next_tag_len] != NULL) {
      p_end = index_g[next_tag_len];
      break;
    }
    ++next_tag_len;
  }
  if (next_tag_len > MAX_TAG_LEN) {
    p_end = map_end_g;
  }

  if (p_db != NULL) {
    while (p_db < p_end) {
#ifdef DEBUG_V
      printf("p_db: %.*s\n", tag_len, p_db);
#endif

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
#ifdef DEBUG_VV
        printf("%c %c\n", *(p_db + 1), *p_input);
#endif
        if (*(++p_db) != *p_input) {
          /* Not matched */
          break;
        }
      }

      if (*(++p_db) == ',' && *p_input == '\0') {
#ifdef DEBUG_V
        printf("Matched\n");
#endif
        /* Matched */
        /* Get lat */
        /* Current
         *     tag,lat,...
         *        ^ *p_db
         *
         * Skip to ','
         *     tag,lat,...
         *            ^ *p_db
         */
        lat = ++p_db;
        lat_len = 1;
        while (*(++p_db) != ',') {
          ++lat_len;
        }

        /* Get lon */
        lon = ++p_db;
        lon_len = 1;
        while (*(++p_db) != ',') {
          ++lon_len;
        }

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
        url_id1_len = 1;
        while (*(++p_db) != ',') {
          ++url_id1_len;
        }

        /* Get id */
        id = ++p_db;
        id_len = 1;
        while (*(++p_db) != ',') {
          ++id_len;
        }

        /* clang-format off */
      len = sprintf(
        buf,
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

        send(TH_PARAM_SOCK, buf, len, MSG_NOSIGNAL);
        result_sep = ',';
      }

      /* Skip to next line */
      while (*(++p_db) != '\n')
        ;
      ++p_db;
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
  FINISH_THREAD();
}

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

  while (1) {
    /* Create a new thread */
    struct sockaddr addr;
    socklen_t len;
    pthread_t th;
    int *p_sock_client;

    p_sock_client = malloc(sizeof(int));
    *p_sock_client = accept(sock_listen, &addr, &len);

    pthread_create(&th, &pth_attr, thread_func, p_sock_client);
  }
}
