#ifndef API_H
#define API_H

#include <curl/curl.h>
#include <curl/easy.h>

typedef struct {
    CURL *curl;
    struct curl_httppost *p_first;
    struct curl_httppost *p_last;
} api_req_t;

api_req_t *req;

int api_init();
void api_cleanup();

int api_create_room();
int api_delete_room();

#endif
