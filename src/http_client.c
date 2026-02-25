/*
 * http_client.c — HTTP client implementation using libcurl
 *
 * Uses thread-local CURL handles for connection pooling and reuse.
 * Each thread gets its own handle on first use, avoiding repeated
 * init/cleanup overhead and enabling TCP/TLS connection reuse.
 */

#include "http_client.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define USER_AGENT "synclyr2metadata (https://github.com/newtonsart/synclyr2metadata)"

/* ── Thread-local CURL handle ─────────────────────────────────────────── */

static __thread CURL *tls_curl = NULL;

/*
 * Get or create the thread-local CURL handle.
 * The handle is reused across all requests within the same thread,
 * enabling HTTP keep-alive and TLS session reuse.
 */
static CURL *get_curl_handle(void)
{
    if (!tls_curl) {
        tls_curl = curl_easy_init();
    }
    return tls_curl;
}

void http_thread_cleanup(void)
{
    if (tls_curl) {
        curl_easy_cleanup(tls_curl);
        tls_curl = NULL;
    }
}

/* ── Internal helpers ─────────────────────────────────────────────────── */

/*
 * libcurl write callback. Appends received data to the response buffer.
 */
static size_t write_callback(char *data, size_t size, size_t nmemb,
                              void *userdata)
{
    size_t real_size = size * nmemb;
    HttpResponse *resp = (HttpResponse *)userdata;

    char *new_body = realloc(resp->body, resp->size + real_size + 1);
    if (!new_body) {
        return 0; /* Signal error to libcurl */
    }

    resp->body = new_body;
    memcpy(resp->body + resp->size, data, real_size);
    resp->size += real_size;
    resp->body[resp->size] = '\0';

    return real_size;
}

/* ── Public API ───────────────────────────────────────────────────────── */

int http_init(void)
{
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    return (res == CURLE_OK) ? 0 : -1;
}

/*
 * Check if a curl error is transient and worth retrying.
 */
static int is_retryable(CURLcode code)
{
    switch (code) {
    case CURLE_COULDNT_CONNECT:
    case CURLE_OPERATION_TIMEDOUT:
    case CURLE_SSL_CONNECT_ERROR:
    case CURLE_GOT_NOTHING:
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:
        return 1;
    default:
        return 0;
    }
}

char *http_url_encode(const char *str)
{
    if (!str) return NULL;

    CURL *curl = get_curl_handle();
    if (!curl) return NULL;

    char *encoded = curl_easy_escape(curl, str, 0);
    char *result  = encoded ? strdup(encoded) : NULL;

    curl_free(encoded);
    return result;
}

HttpResponse *http_get(const char *url)
{
    if (!url) {
        return NULL;
    }

    static const int MAX_RETRIES    = 3;
    static const int BASE_DELAY_SEC = 1; /* 1s, 2s, 4s */

    CURL *curl = get_curl_handle();
    if (!curl) {
        return NULL;
    }

    for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
        HttpResponse *resp = calloc(1, sizeof(HttpResponse));
        if (!resp) {
            return NULL;
        }

        /* Configure the request (reset state from previous use) */
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                              &resp->status_code);
            return resp;
        }

        /* Request failed */
        http_response_free(resp);

        if (attempt < MAX_RETRIES && is_retryable(res)) {
            int delay = BASE_DELAY_SEC << attempt; /* 1, 2, 4 seconds */
            fprintf(stderr, "warning: %s, retrying in %ds (%d/%d)...\n",
                    curl_easy_strerror(res), delay, attempt + 1, MAX_RETRIES);
            struct timespec ts = { .tv_sec = delay, .tv_nsec = 0 };
            nanosleep(&ts, NULL);
        } else {
            fprintf(stderr, "error: HTTP request failed: %s\n",
                    curl_easy_strerror(res));
            return NULL;
        }
    }

    return NULL;
}

void http_response_free(HttpResponse *resp)
{
    if (!resp) {
        return;
    }
    free(resp->body);
    free(resp);
}

void http_cleanup(void)
{
    http_thread_cleanup();
    curl_global_cleanup();
}
