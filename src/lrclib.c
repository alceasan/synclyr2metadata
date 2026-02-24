/*
 * lrclib.c — LRCLIB API client implementation
 *
 * Builds API URLs, performs HTTP requests, and parses JSON responses
 * into LrclibTrack structs.
 */

#define _POSIX_C_SOURCE 200809L

#include "lrclib.h"
#include "http_client.h"
#include "../third_party/cjson/cJSON.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LRCLIB_BASE_URL "https://lrclib.net/api"
#define URL_BUFFER_SIZE 1024

/* ── Internal helpers ──────────────────────────────────────────────────── */

/*
 * Duplicate a string safely. Returns NULL if `src` is NULL.
 */
static char *safe_strdup(const char *src)
{
    return src ? strdup(src) : NULL;
}

/*
 * Get the string value of a cJSON field, or NULL if missing/wrong type.
 */
static const char *json_get_string(const cJSON *obj, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

/*
 * Get the number value of a cJSON field, or `fallback` if missing.
 */
static double json_get_number(const cJSON *obj, const char *key,
                               double fallback)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsNumber(item) ? item->valuedouble : fallback;
}

/*
 * Get the boolean value of a cJSON field, or false if missing.
 */
static bool json_get_bool(const cJSON *obj, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsTrue(item);
}

/*
 * Parse a single JSON object into a heap-allocated LrclibTrack.
 */
static LrclibTrack *parse_track(const cJSON *obj)
{
    if (!cJSON_IsObject(obj)) {
        return NULL;
    }

    LrclibTrack *track = calloc(1, sizeof(LrclibTrack));
    if (!track) {
        return NULL;
    }

    track->id           = (int)json_get_number(obj, "id", 0);
    track->track_name   = safe_strdup(json_get_string(obj, "trackName"));
    track->artist_name  = safe_strdup(json_get_string(obj, "artistName"));
    track->album_name   = safe_strdup(json_get_string(obj, "albumName"));
    track->duration     = json_get_number(obj, "duration", 0.0);
    track->instrumental = json_get_bool(obj, "instrumental");
    track->plain_lyrics = safe_strdup(json_get_string(obj, "plainLyrics"));
    track->synced_lyrics = safe_strdup(json_get_string(obj, "syncedLyrics"));

    return track;
}

/*
 * URL-encode a string using libcurl. Caller must free the result.
 */
static char *url_encode(const char *str)
{
    if (!str) {
        return NULL;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    char *encoded = curl_easy_escape(curl, str, 0);
    char *result  = encoded ? strdup(encoded) : NULL;

    curl_free(encoded);
    curl_easy_cleanup(curl);

    return result;
}

/*
 * Perform a GET request and parse the response as JSON.
 * Returns a cJSON object on success, NULL on failure.
 * Caller must free the result with cJSON_Delete().
 */
static cJSON *api_request(const char *url)
{
    HttpResponse *resp = http_get(url);
    if (!resp) {
        return NULL;
    }

    if (resp->status_code != 200) {
        if (resp->status_code == 404) {
            /* Not found is a valid "no result", not an error */
        } else {
            fprintf(stderr, "error: LRCLIB API returned HTTP %ld\n",
                    resp->status_code);
        }
        http_response_free(resp);
        return NULL;
    }

    cJSON *json = cJSON_Parse(resp->body);
    http_response_free(resp);

    if (!json) {
        fprintf(stderr, "error: failed to parse API response as JSON\n");
    }

    return json;
}

/* ── Public API ────────────────────────────────────────────────────────── */

LrclibTrack *lrclib_get(const char *artist, const char *track,
                         const char *album, double duration)
{
    if (!artist || !track) {
        fprintf(stderr, "error: artist and track are required\n");
        return NULL;
    }

    char *enc_artist = url_encode(artist);
    char *enc_track  = url_encode(track);
    if (!enc_artist || !enc_track) {
        free(enc_artist);
        free(enc_track);
        return NULL;
    }

    char url[URL_BUFFER_SIZE];
    int len = snprintf(url, sizeof(url),
                       "%s/get?artist_name=%s&track_name=%s",
                       LRCLIB_BASE_URL, enc_artist, enc_track);

    /* Append optional parameters */
    if (album) {
        char *enc_album = url_encode(album);
        if (enc_album) {
            snprintf(url + len, sizeof(url) - (size_t)len,
                     "&album_name=%s", enc_album);
            free(enc_album);
        }
    }

    if (duration > 0.0) {
        size_t current_len = strlen(url);
        snprintf(url + current_len, sizeof(url) - current_len,
                 "&duration=%.0f", duration);
    }

    free(enc_artist);
    free(enc_track);

    cJSON *json = api_request(url);
    if (!json) {
        return NULL;
    }

    LrclibTrack *result = parse_track(json);
    cJSON_Delete(json);

    return result;
}

/* ── Memory management ─────────────────────────────────────────────────── */

void lrclib_track_free(LrclibTrack *track)
{
    if (!track) {
        return;
    }
    free(track->track_name);
    free(track->artist_name);
    free(track->album_name);
    free(track->plain_lyrics);
    free(track->synced_lyrics);
    free(track);
}
