#!/bin/bash
#
# lidarr-lyrics.sh — Lidarr Custom Script for synclyr2metadata
#
# Setup in Lidarr:
#   Settings → Connect → + → Custom Script
#     Name:               Sync Lyrics
#     On Release Import:  ✓
#     On Upgrade:         ✓
#     Path:               /config/scripts/lidarr-lyrics.sh
#

BIN="/config/scripts/synclyr2metadata"
LOG="/config/scripts/synclyr2metadata.log"
MAX_LOG_SIZE=102400  # 100KB
THREADS=4

log() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOG"; }

# ── Rotate log if too large ────────────────────────────────────────────

if [ -f "$LOG" ] && [ "$(wc -c < "$LOG")" -gt "$MAX_LOG_SIZE" ]; then
    tail -200 "$LOG" > "$LOG.tmp" && mv "$LOG.tmp" "$LOG"
fi

# ── Auto-install runtime dependencies if missing ───────────────────────

if ! [ -f /usr/lib/libtag_c.so.2 ]; then
    log "Installing runtime dependencies (taglib)..."
    apk add --no-cache taglib >> "$LOG" 2>&1
fi

# ── Only process album imports ─────────────────────────────────────────

case "$lidarr_eventtype" in
    Test)          log "Test OK"; exit 0 ;;
    AlbumDownload) ;;  # continue
    *)             log "Ignoring event: $lidarr_eventtype"; exit 0 ;;
esac

# ── Validate ───────────────────────────────────────────────────────────

[ -z "$lidarr_artist_path" ] && { log "ERROR: lidarr_artist_path not set"; exit 0; }
[ ! -x "$BIN" ]              && { log "ERROR: binary not found at $BIN"; exit 0; }

# ── Find album directory ──────────────────────────────────────────────

ALBUM_DIR=""

# First: extract from the imported file paths
if [ -n "$lidarr_addedtrackpaths" ]; then
    ALBUM_DIR=$(dirname "$(echo "$lidarr_addedtrackpaths" | cut -d'|' -f1)")
fi

# Fallback: match album title under artist path
if [ -z "$ALBUM_DIR" ] || [ ! -d "$ALBUM_DIR" ]; then
    if [ -n "$lidarr_album_title" ]; then
        ALBUM_DIR=$(find "$lidarr_artist_path" -maxdepth 1 -type d \
                    -name "*$lidarr_album_title*" 2>/dev/null | head -1)
    fi
fi

# ── Sync ───────────────────────────────────────────────────────────────

if [ -n "$ALBUM_DIR" ] && [ -d "$ALBUM_DIR" ]; then
    log "Syncing album: $ALBUM_DIR"
    "$BIN" --sync "$ALBUM_DIR" --threads "$THREADS" >> "$LOG" 2>&1
else
    log "Syncing artist: $lidarr_artist_path"
    "$BIN" --artist "$lidarr_artist_path" --threads "$THREADS" >> "$LOG" 2>&1
fi

log "Done"
exit 0
