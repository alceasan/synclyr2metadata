# synclyr2metadata

A lightweight command-line tool that reads local `.lrc` files or automatically downloads synchronized lyrics from [LRCLIB](https://lrclib.net) and **embeds them directly into your audio files' metadata**.

Perfect for use with **Lidarr**, **Navidrome**, **Jellyfin**, **Plex**, or any music player that reads embedded lyrics.

## What does it do?

1. Scans your audio files and reads their metadata (Artist, Title, Album, Duration).
2. Looks for a local `.lrc` sidecar file with the same name. If found, it embeds it directly to save time.
3. If no local file exists, searches for synchronized lyrics on LRCLIB. Falls back to plain lyrics when synced aren't available.
4. Embeds the lyrics permanently into the file's `LYRICS` tag — no separate `.lrc` files needed.

Supports **FLAC**, **MP3**, **OGG**, **M4A/AAC**, **OPUS**, and more (powered by [TagLib](https://taglib.org/)).

---

## 🎵 Lidarr Integration

Auto-sync lyrics every time Lidarr imports a new album.  
**Only one file to deploy** — the binary is fully static with zero runtime dependencies.

---

### 🐳 Option A: Lidarr running in Docker

> Most common setup (Hotio or LinuxServer images on Alpine Linux).

#### 1. Build

```bash
sudo apt install build-essential docker.io   # host prerequisites
git clone https://github.com/newtonsart/synclyr2metadata.git
cd synclyr2metadata
make
```

> Builds a **static binary** (~8MB) inside an Alpine Docker container. No modifications to your `docker-compose.yml` needed.

#### 2. Deploy

Copy the binary into your Lidarr container's config volume:

```bash
cp synclyr2metadata /path/to/lidarr/config/scripts/
```

> Replace `/path/to/lidarr/config` with your actual Lidarr config mount (e.g. `./config` or `/opt/media-stack/config/lidarr`).

#### 3. Enable the Custom Script

1. Open Lidarr UI → **Settings → Connect → + → Custom Script**.
2. Configure:
   - **Name**: `Sync Lyrics`
   - **On Release Import**: ✓
   - **On Upgrade**: ✓
   - **Path**: `/config/scripts/synclyr2metadata`
3. Click **Test**, then **Save**.

Logs are automatically generated in the same folder:
- `/config/scripts/synclyr2metadata.log` (General execution log)
- `/config/scripts/synclyr2metadata_plain.log` (Tracks that only got plain/unsynced lyrics)
- `/config/scripts/synclyr2metadata_missing.log` (Tracks that couldn't be found on LRCLIB)---

### 💻 Option B: Lidarr installed natively

> For bare-metal or systemd-based Lidarr installations.

#### 1. Install dependencies and build

```bash
# Debian / Ubuntu
sudo apt install build-essential libcurl4-openssl-dev libtag1-dev

# Arch
sudo pacman -S base-devel curl taglib

# Then build
git clone https://github.com/newtonsart/synclyr2metadata.git
cd synclyr2metadata
make native
```

#### 2. Deploy

```bash
sudo make install      # installs to /usr/local/bin
```

Or copy manually to a directory Lidarr can access:
```bash
cp synclyr2metadata /path/to/lidarr/scripts/
```

#### 3. Enable the Custom Script

1. Open Lidarr UI → **Settings → Connect → + → Custom Script**.
2. Configure:
   - **Name**: `Sync Lyrics`
   - **On Release Import**: ✓
   - **On Upgrade**: ✓
   - **Path**: `/path/to/synclyr2metadata`
3. Click **Test**, then **Save**.

Logs are automatically generated in the same folder as the binary:
- `synclyr2metadata.log`
- `synclyr2metadata_plain.log`
- `synclyr2metadata_missing.log`

---

## 🛠 Manual CLI Usage

You can also use `synclyr2metadata` directly from the command line:

```bash
# Sync a single album
./synclyr2metadata --album "/path/to/Artist/Album (2024)"

# Sync all albums from an artist
./synclyr2metadata --artist "/path/to/Artist" --threads 8

# Sync your entire library (Artist/Album structure)
./synclyr2metadata --library "/path/to/music" --threads 4

# Export paths of tracks that only got plain lyrics and tracks with missing lyrics
./synclyr2metadata --library "/path/to/music" --out-plain ./plain.txt --out-missing ./missing.txt

# Sync a directory and delete original .lrc sidecar files after embedding them
./synclyr2metadata --album "/path/to/downloaded_album" --clean-lrc
```

### Options

| Option | Description |
|---|---|
| `--album PATH` | Sync lyrics for a single album directory |
| `--artist PATH` | Sync all albums under an artist directory |
| `--library PATH` | Sync entire library (artist/album structure) |
| `--out-plain FILE` | Write paths of tracks falling back to unsynced lyrics to file |
| `--out-missing FILE` | Write paths of tracks not found on LRCLIB to file |
| `--force` | Overwrite existing embedded lyrics |
| `--clean-lrc` | Delete local `.lrc` file after successfully embedding it |
| `--threads N` | Parallel download threads (default: 4, max: 16) |
| `--help` | Show help |

### Example Output

```
═══ MF DOOM ═══

▶ MM.FOOD (2004) (26 tracks)
  [ 1/26] Beef Rapp                                ✓ synced
  [ 2/26] Hoe Cakes                                ✓ plain
  [ 3/26] Potholderz                               ⊘ already has lyrics
  [ 4/26] Unreleased Track                         ✗ not found

──────────────────────────────────────────────
  ✓ Synced:     22
  ✓ Plain:      1
  ⊘ Skipped:    1
  ✗ Not found:  2
──────────────────────────────────────────────
```

---

## License

[GPLv3](LICENSE)

## Credits

- Lyrics by [LRCLIB](https://lrclib.net)
- Built with [libcurl](https://curl.se/), [TagLib](https://taglib.org/), and [cJSON](https://github.com/DaveGamble/cJSON)