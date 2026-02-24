# synclyr2metadata

A lightweight command-line tool that automatically downloads synchronized lyrics (LRC format) from [LRCLIB](https://lrclib.net) and **embeds them directly into your audio files' metadata**.

Perfect for use with **Lidarr**, **Navidrome**, **Jellyfin**, **Plex**, or any modern music player that reads embedded lyrics.

## What does it do?

1. Scans your audio files (reading `Artist`, `Title`, `Album`, and `Duration` metadata).
2. Searches for synchronized lyrics on LRCLIB using a smart two-pass strategy (`Artist` + `Title` first, then falls back to adding `Album` + `Duration` if parsing fails).
3. If lyrics are found, it embeds them permanently into the audio file under the `LYRICS` tag. No separate `.lrc` sidecar files are created.

Supports almost any audio format: **FLAC**, **MP3**, **OGG**, **M4A/AAC**, **OPUS**, etc. (Powered by [TagLib](https://taglib.org/)).

---

## 🚀 Lidarr Docker Integration (Recommended)

Most of us run Lidarr in a Docker container. To make Lidarr automatically download and embed lyrics every time a new album is imported, follow these steps:

### 1. Build the program
First, compile the program on your host machine (outside of Docker):

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install build-essential libcurl4-openssl-dev libtag1-dev

# Clone and build
git clone https://github.com/newtonsart/synclyr2metadata.git
cd synclyr2metadata
make
```

### 2. Copy the files to Lidarr
You now have a binary named `synclyr2metadata` and a shell script `lidarr-lyrics.sh`. Move them to your Lidarr configuration volume (so the container can "see" them):

```bash
# Assuming your Lidarr config volume is mounted at /opt/media-stack/config/lidarr
mkdir -p /opt/media-stack/config/lidarr/scripts
cp synclyr2metadata /opt/media-stack/config/lidarr/scripts/
cp lidarr-lyrics.sh /opt/media-stack/config/lidarr/scripts/
```

### 3. Adjust your docker-compose.yml
The Lidarr container doesn't come with the required libraries pre-installed (`libcurl` and `taglib`). If you use **LinuxServer** or **Hotio** images, you can configure them to install these automatically on boot.

Add the following to the `environment` section of your Lidarr service in your `docker-compose.yml`:

**If you use Hotio (`ghcr.io/hotio/lidarr`):**
```yaml
    environment:
      # Use this for Debian/Ubuntu-based images (like pr-plugins):
      - APTPKGS=libcurl4 taglib-c1v5
      # Use this if your hotio image is Alpine-based:
      # - APKPKGS=curl taglib-c
```

**If you use LinuxServer (`lscr.io/linuxserver/lidarr`):**
```yaml
    environment:
      # LinuxServer images are always Alpine-based
      - DOCKER_MODS=linuxserver/mods:universal-package-install
      - INSTALL_PACKAGES=curl taglib-c
```

Once added, run `docker-compose up -d` to restart Lidarr and install the requirements.

### 4. Enable the Custom Script in Lidarr

1. Open the Lidarr Web UI.
2. Navigate to **Settings → Connect → + → Custom Script**.
3. Fill in the details:
   - **Name**: `Sync Lyrics`
   - **On Release Import**: Enable (✓)
   - **On Upgrade**: Enable (✓)
   - **Path**: `/config/scripts/lidarr-lyrics.sh` *(Note: This is the path INSIDE the container)*
4. Save and click the **Test** button to verify it works.

Optional: You can check the log file located at `/config/scripts/synclyr2metadata.log` inside your config directory to monitor what is being downloaded.

---

## 💻 Manual CLI Usage

If you want to use the tool manually on your computer or server to scan your existing music library, the commands are straightforward.

Run the binary passing one of these three primary options:

### 1. Sync a single album
```bash
./synclyr2metadata --sync "/path/to/music/Artist/Album (2024)"
```

### 2. Sync all albums for an artist
```bash
./synclyr2metadata --artist "/path/to/music/Lil Wayne" --threads 8
```

### 3. Scan your entire library at once
*Note: This assumes your music is organized in simple `Artist / Album` folders.*
```bash
./synclyr2metadata --library "/path/to/music" --threads 10
```

### Extra Options

| Option | Purpose |
|---|---|
| `--force` | Force download and overwrite any existing lyrics you might already have embedded. |
| `--threads N` | The program downloads heavily in parallel using multiple threads. It defaults to 4 threads, but you can crank this up to `16` for huge libraries. |
| `--help` | Shows the quick help manual. |

### Example Output:
```text
═══ MF DOOM

  ▶ MM.FOOD (2004) (26 tracks)
  [ 1/26] Beef Rapp                                ✓ synced
  [ 2/26] One Beer (Madlib remix)                  ✓ synced
  [ 3/26] Hoe Cakes                                ⊘ already has lyrics
  [ 4/26] Unreleased Track                         ✗ not found

──────────────────────────────────────────────
  ✓ Synced:     2
  ⊘ Skipped:    1
  ✗ Not found:  1
──────────────────────────────────────────────
```

## Credits & License

- Lyrics provided by the fantastic, free, and open-source database [LRCLIB](https://lrclib.net).
- Built heavily relying on the open-source libraries [libcurl](https://curl.se/), [TagLib](https://taglib.org/), and [cJSON](https://github.com/DaveGamble/cJSON).
- Open-source software licensed under [GPLv3](LICENSE).