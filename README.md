# synclyr2metadata

A lightweight command-line tool that automatically downloads synchronized lyrics (LRC format) from [LRCLIB](https://lrclib.net) and **embeds them directly into your audio files' metadata**.

Perfect for use with **Lidarr**, **Navidrome**, **Jellyfin**, **Plex**, or any music player that reads embedded lyrics.

## What does it do?

1. Scans your audio files and reads their metadata (Artist, Title, Album, Duration).
2. Searches for synchronized lyrics on LRCLIB. Falls back to plain lyrics when synced aren't available.
3. Embeds the lyrics permanently into the file's `LYRICS` tag — no separate `.lrc` files needed.

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

Logs: `/config/scripts/synclyr2metadata.log` (auto-rotated at 100KB).

---

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

---

## 🛠 Manual CLI Usage

You can also use `synclyr2metadata` directly from the command line:

```bash
# Sync a single album
./synclyr2metadata --sync "/path/to/Artist/Album (2024)"

# Sync all albums from an artist
./synclyr2metadata --artist "/path/to/Artist" --threads 8

# Sync your entire library (Artist/Album structure)
./synclyr2metadata --library "/path/to/music" --threads 4
```

### Options

| Option | Description |
|---|---|
| `--sync PATH` | Sync lyrics for a single album directory |
| `--artist PATH` | Sync all albums under an artist directory |
| `--library PATH` | Sync entire library (artist/album structure) |
| `--force` | Overwrite existing embedded lyrics |
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