/*
 * lrclib.h — Client for the LRCLIB API (https://lrclib.net)
 *
 * Provides functions to retrieve synchronized lyrics
 * from the LRCLIB database.
 */

#ifndef LRCLIB_H
#define LRCLIB_H


/* ── Types ─────────────────────────────────────────────────────────────── */

typedef struct {
    char *synced_lyrics;
    char *plain_lyrics;
} LrclibTrack;

/* ── Public API ────────────────────────────────────────────────────────── */

/*
 * Get the best matching track for the given metadata.
 * `album` and `duration` may be NULL / 0 to omit them.
 *
 * Returns a heap-allocated LrclibTrack on success, NULL if not found
 * or on error. Caller must free with lrclib_track_free().
 */
LrclibTrack *lrclib_get(const char *artist, const char *track,
                         const char *album, double duration);

/* ── Memory management ─────────────────────────────────────────────────── */

void lrclib_track_free(LrclibTrack *track);

#endif /* LRCLIB_H */
