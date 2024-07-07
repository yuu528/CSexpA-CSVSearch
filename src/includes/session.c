#include "session.h"

#include "../../config.h"
#include "tagtypes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *session_thread(void *param) {
  char *buf, *tag;
  int sock = *(int *)param;
  free(param);

  buf = (char *)malloc(RECV_SEND_SIZE * sizeof(char));
  tag = (char *)malloc(MAX_TAG_LEN * sizeof(char));

  if (buf == NULL || tag == NULL) {
    perror(MSG_ERR_MEM_ALLOC);
    RETURN_500(sock);
    FINISH_THREAD_NO_FREE(sock);
  }

  /* Start session */
  if (recv(sock, buf, RECV_SEND_SIZE, 0) <= 0) {
    RETURN_500(sock);
    FINISH_THREAD(sock, buf, tag);
  }

  /* Replace first \r\n with \0 */
  char *p = buf;
  while (*(++p) != '\r') {
    if (*p == '\0') {
      RETURN_500(sock);
      FINISH_THREAD(sock, buf, tag);
    }
  }
  *p = '\0';

  /* Find = */
  p = buf;
  while (*(++p) != '=') {
    if (*p == '\0') {
      RETURN_400(sock);
      FINISH_THREAD(sock, buf, tag);
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
  send(sock, buf, len, MSG_NOSIGNAL);

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

        send(sock, buf, len, MSG_NOSIGNAL);
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
    sock,
    "]}" CRLF,
    2 + CRLF_LEN,
    MSG_NOSIGNAL
  );
  /* clang-format on */

  /* End session */
  FINISH_THREAD(sock, buf, tag);
}
