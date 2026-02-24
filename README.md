# synclyr2metadata

Fetch synchronized lyrics from [LRCLIB](https://lrclib.net) and embed them directly into your audio files' metadata. Built for self-hosted music setups — works out of the box with **Navidrome**, **Jellyfin**, **Plex**, and any player that reads embedded lyrics.

## Features

- 🎵 **Sync a single album, an artist, or your entire library** in one command
- ⚡ **Multi-threaded** — parallel HTTP requests for fast syncing
- 🔄 **Smart matching** — tries artist+track first, falls back to album+duration
- 🛡️ **Retry on failure** — automatic retries with exponential backoff for transient errors
- 📦 **Embeds into metadata** — lyrics go directly into the `LYRICS` tag (no sidecar `.lrc` files)
- 🐳 **Docker support** — mount your music library and sync

## Quick Start

### Build from source

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install build-essential libcurl4-openssl-dev libtag1-dev

# Build
git clone https://github.com/newtonsart/synclyr2metadata.git
cd synclyr2metadata
make
```

### Docker

```bash
docker build -t synclyr2metadata .
docker run --rm -v /path/to/music:/music synclyr2metadata --library /music
```

## Usage

```
synclyr2metadata --sync    "/path/to/album"   [--force] [--threads N]
synclyr2metadata --artist  "/path/to/artist"  [--force] [--threads N]
synclyr2metadata --library "/path/to/music"   [--force] [--threads N]
```

### Options

| Option | Description |
|---|---|
| `--sync` | Sync lyrics for a single album directory |
| `--artist` | Sync all albums under an artist directory |
| `--library` | Sync an entire music library (`artist/album/` structure) |
| `--force` | Overwrite existing embedded lyrics |
| `--threads N` | Number of parallel threads (default: 4, max: 16) |
| `--help` | Show help message |

### Examples

```bash
# Sync a single album
./synclyr2metadata --sync "/data/music/MF DOOM/MM.FOOD (2004)"

# Sync all albums of an artist with 8 threads
./synclyr2metadata --artist "/data/music/Lil Wayne" --threads 8

# Sync your entire library, overwriting existing lyrics
./synclyr2metadata --library /data/music --force --threads 10

```

### Output

```
═══ MF DOOM

  ▶ MM.FOOD (2004) (26 tracks)
  [ 1/26] Beef Rapp                                ✓ synced
  [ 2/26] One Beer (Madlib remix)                  ✓ synced
  [ 3/26] Hoe Cakes                                ⊘ already has lyrics
  ...

──────────────────────────────────────────────
  ✓ Synced:     23
  ⊘ Skipped:    1
  ✗ Not found:  2
──────────────────────────────────────────────
```

## Lidarr Integration

Auto-sync lyrics whenever Lidarr imports an album. In Lidarr:

1. Go to **Settings → Connect → + → Custom Script**
2. Configure:
   - **Name**: `Sync Lyrics`
   - **On Release Import**: ✓
   - **On Upgrade**: ✓
   - **Path**: `/path/to/synclyr2metadata/lidarr-lyrics.sh`
3. Click **Test** to verify, then **Save**

Logs go to `/var/log/synclyr2metadata.log`.

## Supported Formats

Any format supported by [TagLib](https://taglib.org/): **FLAC**, **MP3**, **OGG**, **M4A/AAC**, **OPUS**, **WMA**, **APE**, and more.

Lyrics are stored in the `LYRICS` tag (Vorbis Comment for FLAC/OGG, USLT for MP3), which is the standard tag read by Navidrome, Jellyfin, and most music players.

## Dependencies

| Dependency | Purpose |
|---|---|
| [libcurl](https://curl.se/libcurl/) | HTTP requests to LRCLIB API |
| [TagLib](https://taglib.org/) | Reading/writing audio metadata |
| [cJSON](https://github.com/DaveGamble/cJSON) | JSON parsing (embedded, no install needed) |

### Install dependencies

```bash
# Debian / Ubuntu
sudo apt install build-essential libcurl4-openssl-dev libtag1-dev

# Fedora / RHEL
sudo dnf install gcc make libcurl-devel taglib-devel

# Arch Linux
sudo pacman -S base-devel curl taglib

# Alpine (Docker)
apk add build-base curl-dev taglib-dev
```

## How It Works

1. Scans the directory for audio files and reads their metadata (`artist`, `title`, `album`, `duration`)
2. For each track, queries the [LRCLIB API](https://lrclib.net/docs) for synchronized lyrics
3. If found, embeds the LRC-format lyrics into the file's `LYRICS` metadata tag
4. Retries automatically on transient HTTP errors (SSL, timeout) with exponential backoff

### Matching Strategy

To maximize hit rate, synclyr2metadata uses a two-pass matching strategy:

1. **First attempt**: search by `artist` + `track` only (album names often differ between your tags and LRCLIB)
2. **Fallback**: if no synced lyrics found, retry with `album` + `duration` for a more specific match

## License

[GPLv3](LICENSE)

## Credits

- Lyrics provided by [LRCLIB](https://lrclib.net) — a free, open-source, crowd-sourced lyrics database
- Built with [libcurl](https://curl.se/), [TagLib](https://taglib.org/), and [cJSON](https://github.com/DaveGamble/cJSON)