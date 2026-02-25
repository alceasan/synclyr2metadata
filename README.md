# synclyr2metadata

A lightweight command-line tool that automatically downloads synchronized lyrics (LRC format) from [LRCLIB](https://lrclib.net) and **embeds them directly into your audio files' metadata**.

Perfect for use with **Lidarr**, **Navidrome**, **Jellyfin**, **Plex**, or any music player that reads embedded lyrics.

## What does it do?

1. Scans your audio files and reads their metadata (Artist, Title, Album, Duration).
2. Searches for synchronized lyrics on LRCLIB using a two-pass strategy (Artist + Title first, then falls back to Album + Duration).
3. Embeds the lyrics permanently into the file's `LYRICS` tag. No separate `.lrc` files.

Supports **FLAC**, **MP3**, **OGG**, **M4A/AAC**, **OPUS**, and more (powered by [TagLib](https://taglib.org/)).

---

## 🚀 Lidarr Docker Integration

Auto-sync lyrics every time Lidarr imports a new album.

### 1. Clone and build

The default `make` compiles inside an Alpine Docker container, producing a binary compatible with Lidarr Docker images (Hotio / LinuxServer):

```bash
sudo apt install build-essential   # only needed for make itself
git clone https://github.com/newtonsart/synclyr2metadata.git
cd synclyr2metadata
make
```

> If you prefer to build natively on your host (requires `libcurl-dev` and `libtag1-dev`), use `make native` instead.

### 2. Deploy to Lidarr

Copy the binary and script into your Lidarr config volume:

```bash
mkdir -p /path/to/lidarr/config/scripts
cp synclyr2metadata /path/to/lidarr/config/scripts/
cp lidarr-lyrics.sh /path/to/lidarr/config/scripts/
```

### 3. Enable the Custom Script

1. Open the Lidarr Web UI → **Settings → Connect → + → Custom Script**.
2. Configure:
   - **Name**: `Sync Lyrics`
   - **On Release Import**: ✓
   - **On Upgrade**: ✓
   - **Path**: `/config/scripts/lidarr-lyrics.sh`
3. Click **Test**, then **Save**.

Logs are written to `/config/scripts/synclyr2metadata.log`.

---

## 💻 Manual CLI Usage

```bash
# Sync a single album
./synclyr2metadata --sync "/path/to/Artist/Album (2024)"

# Sync all albums from an artist
./synclyr2metadata --artist "/path/to/Artist" --threads 8

# Sync your entire library (Artist/Album structure)
./synclyr2metadata --library "/path/to/music" --threads 10
```

### Options

| Option | Description |
|---|---|
| `--force` | Overwrite existing embedded lyrics |
| `--threads N` | Parallel download threads (default: 4, max: 16) |
| `--help` | Show help |

### Example Output

```
═══ MF DOOM ═══

▶ MM.FOOD (2004) (26 tracks)
  [ 1/26] Beef Rapp                                ✓ synced
  [ 2/26] Hoe Cakes                                ⊘ already has lyrics
  [ 3/26] Unreleased Track                         ✗ not found

──────────────────────────────────────────────
  ✓ Synced:     23
  ⊘ Skipped:    1
  ✗ Not found:  2
──────────────────────────────────────────────
```

## License

[GPLv3](LICENSE)

## Credits

- Lyrics by [LRCLIB](https://lrclib.net)
- Built with [libcurl](https://curl.se/), [TagLib](https://taglib.org/), and [cJSON](https://github.com/DaveGamble/cJSON)