/*
 * lrclib.c — LRCLIB API client implementation
 *
 * Builds API URLs, performs HTTP requests, and parses JSON responses
 * into LrclibTrack structs.
 */

#include "lrclib.h"
#include "http_client.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LRCLIB_BASE_URL "https://lrclib.net/api"
#define URL_BUFFER_SIZE 1024

/* ── Internal helpers ──────────────────────────────────────────────────── */

/*
 * Get the string value of a cJSON field, or NULL if missing/wrong type.
 */
static const char *json_get_string(const cJSON *obj, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

/*
 * Parse a JSON object into a heap-allocated LrclibTrack.
 * Only extracts the lyrics fields we actually use.
 */
static LrclibTrack *parse_track(const cJSON *obj)
{
    if (!cJSON_IsObject(obj)) {
        return NULL;
    }

    const char *synced = json_get_string(obj, "syncedLyrics");
    const char *plain  = json_get_string(obj, "plainLyrics");
    cJSON *inst_item   = cJSON_GetObjectItemCaseSensitive(obj, "instrumental");

    LrclibTrack *track = calloc(1, sizeof(LrclibTrack));
    if (!track) {
        return NULL;
    }

    track->synced_lyrics = synced ? strdup(synced) : NULL;
    track->plain_lyrics  = plain  ? strdup(plain)  : NULL;
    track->instrumental  = cJSON_IsTrue(inst_item) ? 1 : 0;

    return track;
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

    char *enc_artist = http_url_encode(artist);
    char *enc_track  = http_url_encode(track);
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
        char *enc_album = http_url_encode(album);
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
    free(track->synced_lyrics);
    free(track->plain_lyrics);
    free(track);
}
