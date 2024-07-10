#ifndef _H_CONFIG_

#define _H_CONFIG_

/* Debug options */
// #define DEBUG
// #define DEBUG_V
// #define DEBUG_VV

/* Thread settings */
#define PRE_THREAD
#define PRE_THREAD_COUNT 4
#define ACCEPT_ON_CHILD /* accept() on child or main thread */

/* File load settings */
/* Use read() instead of mmap() */
#define USE_READ

/* Parsing settings */
// #define DISABLE_URL_DECODE /* Disable URL decoding */
#define ALT_URL_DECODE /* Use alternative method to decode URL */
// #define DISABLE_ESCAPE /* Disable escape characters */
#define ALT_INDEX /* Use alternative method to create index */

/* Connection settings */
#define CHECK_EPOLL_ERROR         /* Check epoll_create() error */
#define CHECK_ACCEPT_ERROR        /* Check accept() error */
#define CHECK_THREAD_CREATE_ERROR /* Check pthread_create() error */
#define CHECK_RECV_LENGTH         /* Check recv() length */
#define CHECK_QUERY_EQUAL         /* Check query string '=' */
#define CHECK_SEND_ERROR          /* Check send() error */
// #define ENABLE_TIMEOUT
#define ENABLE_TCP_NODELAY
#define ENABLE_TCP_CORK
#define DISABLE_LINGER
// #define SEND_NONBLOCK
/* Send, recv timeout */
#define TIMEOUT_SEC 1
/* Skip first 'GET /?tag' */
#define SKIP_HEADER_FIRST_LEN 9
/* Use epoll for accept() */
#define USE_EPOLL
/* First skip chars when not matched */
/*
 * tag,lat,lon,YYYYMMDDHHMMSSserver_id,URL_ID1,ID,URL_ID2
 *   ^ ---------> Move <SKIP_TO_NEXT_LINE_FIRST>
 */
#define SKIP_TO_NEXT_LINE_FIRST 25
// #define USE_LARGE_BUFFER

/* Length settings */
#define MAX_TAG_LEN 370
#define MAX_TAG_LEN_DECODED 285
#define MAX_TAG_LEN_ESCAPED MAX_TAG_LEN_DECODED + 6
#define MAX_GEOTAG_PER_TAG 100
#define MAX_LAT_LON_LEN 16
#define YEAR_LEN 4
#define MONTH_LEN 2
#define DAY_LEN 2
#define HOUR_LEN 2
#define MINUTE_LEN 2
#define SECOND_LEN 2
#define SERVER_ID_LEN 1
#define MAX_URL_ID1_LEN 4
#define MAX_ID_LEN 10
#define URL_ID2_LEN 10

/* Buffer settings */
#define FILE_BUFFER_SIZE 512
#define RECV_SEND_SIZE 2048
#define RECV_SEND_SIZE_LARGE 356 + 137 * MAX_GEOTAG_PER_TAG

/* File delimiter settings */
#define CSV_DELIM ","
#define DATE_TIME_DELIM " "
#define DATE_DELIM "-"
#define TIME_DELIM ":"

/* Message settings */
#define MSG_INFO_LOADING "Loading data..."
#define MSG_INFO_CREATE_INDEX "Creating index..."
#define MSG_INFO_DONE "Done."

#define MSG_ERR_FILE_OPEN "File open error"
#define MSG_ERR_LEN "Length error"
#define MSG_ERR_MEM_ALLOC "Memory allocation error"
#define MSG_ERR_FILE_STAT "File stat error"
#define MSG_ERR_MMAP "Memory map error"
#define MSG_ERR_EPOLL_CREATE "Epoll create error"
#define MSG_ERR_EPOLL_WAIT "Epoll wait error"
#define MSG_ERR_ACCEPT "Accept error"
#define MSG_ERR_THREAD_CREATE "Thread create error"

/* Reply settings */
#define HEADER_200 "HTTP/1.1 200 OK"
#define HEADER_200_LEN 15

#define HEADER_400 "HTTP/1.1 400 Bad Request"
#define HEADER_400_LEN 24

#define HEADER_500 "HTTP/1.1 500 Internal Server Error"
#define HEADER_500_LEN 33

#define HEADER_CONTENT_LENGTH "Content-Length: "
#define HEADER_CONTENT_LENGTH_LEN 16

#define HEADER_CONTENT_TYPE "Content-Type: "
#define HEADER_CONTENT_TYPE_LEN 14

#define CRLF "\r\n"
#define CRLF_LEN 2

#define MIME_JSON "application/json"
#define MIME_JSON_LEN 16

#define JSON_KEY_TAG "\"tag\""
#define JSON_KEY_RESULTS "\"results\""
#define JSON_KEY_LAT "\"lat\""
#define JSON_KEY_LON "\"lon\""
#define JSON_KEY_DATE "\"date\""
#define JSON_KEY_URL "\"url\""

#endif
