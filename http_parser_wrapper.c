#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_parser.h"
#include "http_parser_wrapper.h"

#define MAX_HTTP_HEADERS 8

struct http_header {
    char *key;
    char *value;
};

struct http_headers {
    size_t capacity;
    size_t count;
    struct http_header *headers;
    int complete;
    char *url;
    size_t content_beginning;
    uint8_t *origin_data; /* weak pointer */
    size_t origin_data_len;
};


static int on_message_begin(http_parser* p) {
    return 0;
}

static int on_headers_complete(http_parser* parser) {
    struct http_headers *hdrs = (struct http_headers *) parser->data;
    hdrs->complete = 1;
    return 0;
}

static int on_message_complete(http_parser* p) {
    return 0;
}

static int on_chunk_header(http_parser* p) {
    return 0;
}

static int on_chunk_complete(http_parser* p) {
    return 0;
}

static int on_url(http_parser *parser, const char *at, size_t length) {
    struct http_headers *parsed_headers = (struct http_headers *)parser->data;
    parsed_headers->url = (char *) calloc(length + 1, sizeof(char));
    strncpy(parsed_headers->url, at, length);
    return 0;
}

static int on_status(http_parser* p, const char *at, size_t length) {
    return 0;
}

static int on_header_field(http_parser* parser, const char *at, size_t length) {
    struct http_headers *parsed_headers = (struct http_headers *)parser->data;
    char *key;

    if (parsed_headers->count >= parsed_headers->capacity) {
        if (parsed_headers->capacity == 0) {
            assert(parsed_headers->headers == NULL);
            parsed_headers->capacity = MAX_HTTP_HEADERS;
            parsed_headers->headers = (struct http_header *)
                calloc(parsed_headers->capacity, sizeof(struct http_header));
        } else {
            parsed_headers->capacity *= 2;
            parsed_headers->headers = (struct http_header *)
                realloc(parsed_headers->headers, parsed_headers->capacity * sizeof(struct http_header));
        }
    }

    key = (char *) calloc(1, length + 1);
    strncpy(key, at, length);

    parsed_headers->headers[parsed_headers->count].key = key;

    return 0;
}

static int on_header_value(http_parser* parser, const char *at, size_t length) {
    struct http_headers *parsed_headers = (struct http_headers *) parser->data;

    char *value = (char *) calloc(1, length + 1);
    strncpy(value, at, length);

    parsed_headers->headers[parsed_headers->count].value = value;
    parsed_headers->count++;

    return 0;
}

static int on_body(http_parser* p, const char *at, size_t length) {
    struct http_headers *hdrs = (struct http_headers *) p->data;
    hdrs->content_beginning = (uint8_t *)at - hdrs->origin_data;
    // assert(hdrs->origin_data_len == length + hdrs->content_beginning);
    return 0;
}

size_t http_heads_count(struct http_headers *headers) {
    return headers->count;
}

const char * http_heads_get_url(struct http_headers *headers) {
    return headers->url;
}

void enumerate_http_headers(struct http_headers *headers, header_walker cb, void *p) {
    size_t i;
    if (headers==NULL || cb==NULL) {
        return;
    }
    for(i = 0; i < headers->count; i++) {
        struct http_header *h = headers->headers + i;
        int stop = 0;
        cb(h->key, h->value, &stop, p);
        if (stop) { break; }
    }
}

void get_header_val_cb(char *key, char *value, int *stop, void *p) {
    struct http_header *data = (struct http_header*)p;
    if (strcmp(key, data->key) == 0) {
        data->value = value;
        if (stop) { *stop = 1; }
    }
}

const char * get_header_val(const struct http_headers *headers, const char *header_key) {
    struct http_header data = { (char *)header_key, NULL };
    enumerate_http_headers((struct http_headers *)headers, get_header_val_cb, &data);
    return data.value;
}

size_t get_http_content_beginning(const struct http_headers *headers) {
    return headers->content_beginning;
}

void destroy_http_headers_cb(char *key, char *value, int *stop, void *p) {
    free(key);
    free(value);
}

void destroy_http_headers(struct http_headers *headers) {
    if (headers == NULL) {
        return;
    }
    if (headers->url) {
        free(headers->url);
    }
    if (headers->headers) {
        enumerate_http_headers(headers, destroy_http_headers_cb, NULL);
        free(headers->headers);
    }
    free(headers);
}

static http_parser_settings settings = {
    on_message_begin,
    on_url,
    on_status,
    on_header_field,
    on_header_value,
    on_headers_complete,
    on_body,
    on_message_complete,
    on_chunk_header,
    on_chunk_complete,
};

struct http_headers * parse_http_heads(int request, const uint8_t *data, size_t data_len) {
    struct http_parser parser = { 0 };
    size_t parsed;
    struct http_headers *parsed_headers;
    parsed_headers = (struct http_headers *) calloc(1, sizeof(struct http_headers));
    parsed_headers->origin_data = (uint8_t *)data;
    parsed_headers->origin_data_len = data_len;
    parsed_headers->content_beginning = data_len;
    parser.data = parsed_headers;
    http_parser_init(&parser, request ? HTTP_REQUEST : HTTP_RESPONSE);

    parsed = http_parser_execute(&parser, &settings, (char *)data, data_len);
    assert(parsed == data_len);
    return parsed_headers;
}
