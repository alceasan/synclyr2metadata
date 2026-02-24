/*
 * http_client.h — HTTP client abstraction over libcurl
 *
 * Provides a simple interface for making HTTP GET requests.
 * Manages memory allocation for response bodies internally.
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>

/* ── Types ─────────────────────────────────────────────────────────────── */

typedef struct {
    char  *body;        /* Response body (null-terminated)   */
    size_t size;        /* Body length in bytes              */
    long   status_code; /* HTTP status code (e.g. 200, 404)  */
} HttpResponse;

/* ── Public API ────────────────────────────────────────────────────────── */

/*
 * Initialize the HTTP subsystem. Must be called once before any other
 * http_* function. Returns 0 on success, -1 on failure.
 */
int http_init(void);

/*
 * Perform an HTTP GET request to `url`.
 * Returns a heap-allocated HttpResponse on success, NULL on failure.
 * Caller must free the response with http_response_free().
 */
HttpResponse *http_get(const char *url);

/*
 * Free an HttpResponse previously returned by http_get().
 * Safe to call with NULL.
 */
void http_response_free(HttpResponse *resp);

/*
 * Clean up the HTTP subsystem. Call once at program exit.
 */
void http_cleanup(void);

#endif /* HTTP_CLIENT_H */
