#ifndef _H_CONFIG_

#define _H_CONFIG_

/* Debug options */
#define DEBUG
// #define DEBUG_V
// #define DEBUG_VV

/* Length settings */
#define MAX_TAG_LEN 255
#define MAX_GEOTAG_PER_TAG 100

/* Buffer settings */
#define FILE_BUFFER_SIZE 512
#define RECV_SEND_SIZE 1024

/* File delimiter settings */
#define CSV_DELIM ","
#define DATE_TIME_DELIM " "
#define DATE_DELIM "-"
#define TIME_DELIM ":"

/* Message settings */
#define MSG_ERR_FILE_OPEN "File open error\n"
#define MSG_ERR_MEM_ALLOC "Memory allocation error\n"

/* Reply settings */
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

#define JSON_KEY_TAG "\"tag\""
#define JSON_KEY_RESULTS "\"results\""
#define JSON_KEY_LAT "\"lat\""
#define JSON_KEY_LON "\"lon\""
#define JSON_KEY_DATE "\"date\""
#define JSON_KEY_URL "\"url\""

#endif
