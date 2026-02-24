#!/bin/bash
#
# lidarr-lyrics.sh — Lidarr Custom Script for synclyr2metadata
#
# Setup in Lidarr:
#   Settings → Connect → + → Custom Script
#     Name:          Sync Lyrics
#     On Release Import:  ✓
#     On Upgrade:         ✓
#     Path:          /path/to/synclyr2metadata/lidarr-lyrics.sh
#
# Environment variables set by Lidarr:
#   lidarr_eventtype       = Download | AlbumDownload | Test
#   lidarr_artist_path     = /data/media/music/Artist Name
#   lidarr_album_title     = Album Name
#

# Default docker container paths for Lidarr
BIN="/config/scripts/synclyr2metadata"
LOG="/config/scripts/synclyr2metadata.log"
THREADS=8

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOG"
}

# Lidarr sends a Test event to verify the script works
if [ "$lidarr_eventtype" = "Test" ]; then
    log "Test event received — script is working"
    exit 0
fi

# Validate
if [ -z "$lidarr_artist_path" ]; then
    log "ERROR: lidarr_artist_path not set"
    exit 0  # exit 0 so Lidarr doesn't mark it as failed
fi

if [ ! -x "$BIN" ]; then
    log "ERROR: synclyr2metadata not found at $BIN"
    exit 0
fi

# Find the album directory
# Lidarr gives us artist_path + album_title, build the album path
ALBUM_DIR=""

if [ -n "$lidarr_album_title" ]; then
    # Try to find a matching directory
    while IFS= read -r dir; do
        ALBUM_DIR="$dir"
        break
    done < <(find "$lidarr_artist_path" -maxdepth 1 -type d -name "*$lidarr_album_title*" 2>/dev/null)
fi

# Fallback: if no specific album found, sync the entire artist directory
if [ -z "$ALBUM_DIR" ] || [ ! -d "$ALBUM_DIR" ]; then
    log "Syncing all albums for artist: $lidarr_artist_path"
    "$BIN" --artist "$lidarr_artist_path" --force --threads "$THREADS" >> "$LOG" 2>&1
else
    log "Syncing album: $ALBUM_DIR"
    "$BIN" --sync "$ALBUM_DIR" --force --threads "$THREADS" >> "$LOG" 2>&1
fi

log "Done"
exit 0
