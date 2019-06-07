#ifndef __http_parser_wrapper_h__
#define __http_parser_wrapper_h__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#if defined(_WIN32) && !defined(__MINGW32__) && \
  (!defined(_MSC_VER) || _MSC_VER<1600) && !defined(__WINE__)
#include <BaseTsd.h>
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

struct http_headers;

struct http_headers * parse_http_heads(int request, const uint8_t *data, size_t data_len);

size_t http_heads_count(struct http_headers *headers);

const char * http_heads_get_url(struct http_headers *headers);
const char * http_heads_get_status(struct http_headers *headers);

typedef void(*header_walker)(char *key, char *value, int *stop, void *p);
void enumerate_http_headers(struct http_headers *headers, header_walker cb, void *p);

const char * get_header_val(const struct http_headers *headers, const char *header_key);

size_t get_http_content_beginning(const struct http_headers *headers);

void destroy_http_headers(struct http_headers *headers);

#ifdef __cplusplus
}
#endif
#endif /* __http_parser_wrapper_h__ */
