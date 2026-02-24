/*
 * main.c — CLI entry point for synclyr2metadata
 *
 * Usage:
 *   synclyr2metadata --sync    "/path/to/album"
 *   synclyr2metadata --artist  "/path/to/artist"
 *   synclyr2metadata --library "/path/to/music"
 *   synclyr2metadata --scan    "/path/to/album"
 */

#include "http_client.h"
#include "lrclib.h"
#include "metadata.h"

#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ── Usage ─────────────────────────────────────────────────────────────── */

static void print_usage(const char *progname)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s --sync    \"/path/to/album\"   [--force] [--threads N]\n"
        "  %s --artist  \"/path/to/artist\"  [--force] [--threads N]\n"
        "  %s --library \"/path/to/music\"   [--force] [--threads N]\n"
        "\n"
        "Options:\n"
        "  --sync     Sync lyrics for a single album directory\n"
        "  --artist   Sync lyrics for all albums of an artist\n"
        "  --library  Sync lyrics for an entire library (artist/album)\n"
        "  --force    Overwrite existing lyrics\n"
        "  --threads  Number of parallel threads (default: 4, max: 16)\n"
        "  --help     Show this help message\n",
        progname, progname, progname);
}


/* ── Sync logic (parallel) ─────────────────────────────────────────────── */

#define SYNC_DEFAULT_THREADS 4

typedef struct {
    const TrackMetaList *list;
    int                  force;
    int                  next_index;
    int                  synced;
    int                  skipped;
    int                  not_found;
    int                  errors;
    pthread_mutex_t      mutex;
} SyncContext;

static void process_track(const TrackMeta *t, int force,
                          int *out_synced, int *out_skipped,
                          int *out_not_found, int *out_error,
                          const char **out_status)
{
    *out_synced = 0;
    *out_skipped = 0;
    *out_not_found = 0;
    *out_error = 0;

    if (!t->artist || !t->title) {
        *out_not_found = 1;
        *out_status = "\xe2\x9c\x97 missing metadata";
        return;
    }

    if (!force && metadata_has_lyrics(t->filepath) == 1) {
        *out_skipped = 1;
        *out_status = "\xe2\x8a\x98 already has lyrics";
        return;
    }

    LrclibTrack *lrc = lrclib_get(t->artist, t->title, NULL, 0);

    if ((!lrc || !lrc->synced_lyrics || lrc->synced_lyrics[0] == '\0')
        && t->album) {
        lrclib_track_free(lrc);
        lrc = lrclib_get(t->artist, t->title, t->album, (double)t->duration);
    }

    if (!lrc || !lrc->synced_lyrics || lrc->synced_lyrics[0] == '\0') {
        *out_not_found = 1;
        *out_status = "\xe2\x9c\x97 not found";
        lrclib_track_free(lrc);
        return;
    }

    if (metadata_write_lyrics(t->filepath, lrc->synced_lyrics) == 0) {
        *out_synced = 1;
        *out_status = "\xe2\x9c\x93 synced";
    } else {
        *out_error = 1;
        *out_status = "\xe2\x9c\x97 write error";
    }

    lrclib_track_free(lrc);
}

static void *sync_worker(void *arg)
{
    SyncContext *ctx = (SyncContext *)arg;

    for (;;) {
        pthread_mutex_lock(&ctx->mutex);
        int idx = ctx->next_index++;
        pthread_mutex_unlock(&ctx->mutex);

        if (idx >= ctx->list->count) {
            break;
        }

        const TrackMeta *t = ctx->list->items[idx];

        int s = 0, sk = 0, nf = 0, e = 0;
        const char *status = "";
        process_track(t, ctx->force, &s, &sk, &nf, &e, &status);

        pthread_mutex_lock(&ctx->mutex);
        printf("  [%2d/%d] %-40.40s %s\n",
               idx + 1, ctx->list->count,
               t->title ? t->title : "(unknown)", status);
        ctx->synced    += s;
        ctx->skipped   += sk;
        ctx->not_found += nf;
        ctx->errors    += e;
        pthread_mutex_unlock(&ctx->mutex);
    }

    return NULL;
}

/*
 * Sync a single album directory. Returns counts via out params.
 */
static int sync_directory(const char *dirpath, int force, int num_threads,
                          int *total_s, int *total_sk, int *total_nf, int *total_e)
{
    TrackMetaList *list = metadata_scan_dir(dirpath);
    if (!list) {
        return 1;
    }

    if (list->count == 0) {
        metadata_list_free(list);
        return 0;
    }

    int t = num_threads > list->count ? list->count : num_threads;

    SyncContext ctx = {
        .list       = list,
        .force      = force,
        .next_index = 0,
        .synced     = 0,
        .skipped    = 0,
        .not_found  = 0,
        .errors     = 0,
    };
    pthread_mutex_init(&ctx.mutex, NULL);

    pthread_t *threads = calloc((size_t)t, sizeof(pthread_t));
    for (int i = 0; i < t; i++) {
        pthread_create(&threads[i], NULL, sync_worker, &ctx);
    }
    for (int i = 0; i < t; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    pthread_mutex_destroy(&ctx.mutex);

    *total_s  += ctx.synced;
    *total_sk += ctx.skipped;
    *total_nf += ctx.not_found;
    *total_e  += ctx.errors;

    metadata_list_free(list);
    return 0;
}

/*
 * Check if a path is a directory.
 */
static int is_directory(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

/*
 * Print the totals summary.
 */
static void print_summary(int synced, int skipped, int not_found, int errors)
{
    printf("\n\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n");
    printf("  \xe2\x9c\x93 Synced:     %d\n", synced);
    printf("  \xe2\x8a\x98 Skipped:    %d\n", skipped);
    printf("  \xe2\x9c\x97 Not found:  %d\n", not_found);
    if (errors > 0) {
        printf("  \xe2\x9c\x97 Errors:     %d\n", errors);
    }
    printf("\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n");
}

/* ── Sync modes ────────────────────────────────────────────────────────── */

/*
 * --sync: sync a single album directory
 */
static int cmd_sync(const char *dirpath, int force, int num_threads)
{
    TrackMetaList *list = metadata_scan_dir(dirpath);
    if (!list || list->count == 0) {
        if (list) metadata_list_free(list);
        printf("No audio files found in '%s'.\n", dirpath);
        return 0;
    }
    int count = list->count;
    metadata_list_free(list);

    printf("Syncing lyrics for %d track(s) in '%s' [%d threads]...\n\n",
           count, dirpath, num_threads);

    int s = 0, sk = 0, nf = 0, e = 0;
    sync_directory(dirpath, force, num_threads, &s, &sk, &nf, &e);
    print_summary(s, sk, nf, e);

    return (e > 0) ? 1 : 0;
}

/*
 * --artist: sync all album subdirectories under an artist
 */
static int cmd_artist(const char *artist_path, int force, int num_threads)
{
    DIR *dir = opendir(artist_path);
    if (!dir) {
        fprintf(stderr, "error: could not open '%s'\n", artist_path);
        return 1;
    }

    const char *artist_name = strrchr(artist_path, '/');
    artist_name = artist_name ? artist_name + 1 : artist_path;

    printf("\u2550\u2550\u2550 %s \u2550\u2550\u2550\n\n", artist_name);

    int s = 0, sk = 0, nf = 0, e = 0;
    int album_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        size_t len = strlen(artist_path) + strlen(entry->d_name) + 2;
        char *sub = malloc(len);
        if (!sub) continue;
        snprintf(sub, len, "%s/%s", artist_path, entry->d_name);

        if (!is_directory(sub)) {
            free(sub);
            continue;
        }

        /* Check if this subdir has audio files */
        TrackMetaList *list = metadata_scan_dir(sub);
        if (!list || list->count == 0) {
            if (list) metadata_list_free(list);
            free(sub);
            continue;
        }

        album_count++;
        printf("\u25b6 %s (%d tracks)\n", entry->d_name, list->count);
        metadata_list_free(list);

        sync_directory(sub, force, num_threads, &s, &sk, &nf, &e);
        printf("\n");
        free(sub);
    }

    closedir(dir);

    if (album_count == 0) {
        printf("No albums found.\n");
        return 0;
    }

    printf("%d album(s) processed", album_count);
    print_summary(s, sk, nf, e);

    return (e > 0) ? 1 : 0;
}

/*
 * --library: scan artist/album structure and sync everything
 */
static int cmd_library(const char *library_path, int force, int num_threads)
{
    DIR *dir = opendir(library_path);
    if (!dir) {
        fprintf(stderr, "error: could not open '%s'\n", library_path);
        return 1;
    }

    printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
    printf("  synclyr2metadata \u2014 Library Sync\n");
    printf("  Path:     %s\n", library_path);
    printf("  Threads:  %d\n", num_threads);
    printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n\n");

    int total_s = 0, total_sk = 0, total_nf = 0, total_e = 0;
    int artist_count = 0, album_count = 0;

    /* Iterate artists */
    struct dirent *artist_entry;
    while ((artist_entry = readdir(dir)) != NULL) {
        if (artist_entry->d_name[0] == '.') continue;

        size_t alen = strlen(library_path) + strlen(artist_entry->d_name) + 2;
        char *artist_dir = malloc(alen);
        if (!artist_dir) continue;
        snprintf(artist_dir, alen, "%s/%s", library_path, artist_entry->d_name);

        if (!is_directory(artist_dir)) {
            free(artist_dir);
            continue;
        }

        /* Iterate albums under this artist */
        DIR *adir = opendir(artist_dir);
        if (!adir) {
            free(artist_dir);
            continue;
        }

        int artist_albums = 0;

        struct dirent *album_entry;
        while ((album_entry = readdir(adir)) != NULL) {
            if (album_entry->d_name[0] == '.') continue;

            size_t blen = strlen(artist_dir) + strlen(album_entry->d_name) + 2;
            char *album_dir = malloc(blen);
            if (!album_dir) continue;
            snprintf(album_dir, blen, "%s/%s", artist_dir, album_entry->d_name);

            if (!is_directory(album_dir)) {
                free(album_dir);
                continue;
            }

            TrackMetaList *list = metadata_scan_dir(album_dir);
            if (!list || list->count == 0) {
                if (list) metadata_list_free(list);
                free(album_dir);
                continue;
            }

            if (artist_albums == 0) {
                printf("\u2550\u2550\u2550 %s\n", artist_entry->d_name);
            }

            artist_albums++;
            album_count++;
            printf("  \u25b6 %s (%d tracks)\n", album_entry->d_name, list->count);
            metadata_list_free(list);

            sync_directory(album_dir, force, num_threads,
                           &total_s, &total_sk, &total_nf, &total_e);
            free(album_dir);
        }

        closedir(adir);

        if (artist_albums > 0) {
            artist_count++;
            printf("\n");
        }

        free(artist_dir);
    }

    closedir(dir);

    printf("\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\n");
    printf("  Library Sync Complete\n");
    printf("  Artists:  %d\n", artist_count);
    printf("  Albums:   %d\n", album_count);
    print_summary(total_s, total_sk, total_nf, total_e);

    return (total_e > 0) ? 1 : 0;
}

/* ── Argument parsing ──────────────────────────────────────────────────── */

static const char *find_arg(int argc, char **argv, const char *flag)
{
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return argv[i + 1];
        }
    }
    return NULL;
}

static int has_flag(int argc, char **argv, const char *flag)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ── Main ──────────────────────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    if (argc < 2 || has_flag(argc, argv, "--help")) {
        print_usage(argv[0]);
        return (argc < 2) ? 1 : 0;
    }

    int exit_code = 0;
    int force = has_flag(argc, argv, "--force");

    const char *threads_str = find_arg(argc, argv, "--threads");
    int num_threads = threads_str ? atoi(threads_str) : SYNC_DEFAULT_THREADS;
    if (num_threads < 1) num_threads = 1;
    if (num_threads > 16) num_threads = 16;

    const char *sync_dir    = find_arg(argc, argv, "--sync");
    const char *artist_dir  = find_arg(argc, argv, "--artist");
    const char *library_dir = find_arg(argc, argv, "--library");

    if (sync_dir || artist_dir || library_dir) {
        /* ── All sync modes require HTTP ─────────────────────────── */
        if (http_init() != 0) {
            fprintf(stderr, "error: failed to initialize HTTP client\n");
            return 1;
        }

        if (library_dir) {
            exit_code = cmd_library(library_dir, force, num_threads);
        } else if (artist_dir) {
            exit_code = cmd_artist(artist_dir, force, num_threads);
        } else {
            exit_code = cmd_sync(sync_dir, force, num_threads);
        }

        http_cleanup();

    } else {
        fprintf(stderr, "error: invalid arguments\n\n");
        print_usage(argv[0]);
        exit_code = 1;
    }

    return exit_code;
}
