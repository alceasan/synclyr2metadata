# synclyr2metadata

Un pequeño programa de consola que descarga automáticamente las letras sincronizadas (formato LRC) desde [LRCLIB](https://lrclib.net) y las **incrusta directamente dentro de tus archivos de música**.

Perfecto para usar con **Lidarr**, **Navidrome**, **Jellyfin**, **Plex**, o cualquier reproductor que lea letras desde los metadatos de las canciones.

## ¿Qué hace exactamente?

1. Escanea tu música (lee los metadatos como Artista, Título, Álbum y Duración).
2. Busca la letra sincronizada en LRCLIB usando un sistema de doble pasada (Artista+Título primero, y si falla, incluye Álbum+Duración).
3. Si la encuentra, la guarda permanentemente dentro del propio archivo de música en el tag `LYRICS`. No crea archivos `.lrc` separados.

Soporta casi cualquier formato: **FLAC**, **MP3**, **OGG**, **M4A/AAC**, **OPUS**, etc. (Gracias a [TagLib](https://taglib.org/)).

---

## 🚀 Integración con Lidarr en Docker (Recomendado)

La inmensa mayoría de usuarios tenemos Lidarr dentro de un contenedor Docker. Para que Lidarr descargue y meta las letras automáticamente cada vez que importa un álbum nuevo, sigue estos pasos:

### 1. Compilar el programa
Primero necesitas compilar el programa en tu máquina host (fuera de Docker):

```bash
# Instala las dependencias en tu Linux (Debian/Ubuntu)
sudo apt install build-essential libcurl4-openssl-dev libtag1-dev

# Clona y compila
git clone https://github.com/newtonsart/synclyr2metadata.git
cd synclyr2metadata
make
```

### 2. Copiar los archivos a Lidarr
Ahora tienes un archivo binario llamado `synclyr2metadata` y un script llamado `lidarr-lyrics.sh`. Tienes que moverlos a la carpeta de configuración de tu Lidarr (para que el contenedor pueda "verlos"):

```bash
# Asumiendo que tu config de Lidarr está en /opt/media-stack/config/lidarr
mkdir -p /opt/media-stack/config/lidarr/scripts
cp synclyr2metadata /opt/media-stack/config/lidarr/scripts/
cp lidarr-lyrics.sh /opt/media-stack/config/lidarr/scripts/
```

### 3. Ajustar tu docker-compose.yml
El contenedor de Lidarr no viene con las librerías necesarias de serie. Si usas las imágenes de **LinuxServer** o **Hotio**, puedes hacer que las instale automáticamente al arrancar.

Añade esto a los `environment` de tu Lidarr en tu `docker-compose.yml`:

**Si usas Hotio (`ghcr.io/hotio/lidarr`):**
```yaml
    environment:
      # Dependiendo de si es la versión Alpine o Debian/Ubuntu:
      - APTPKGS=libcurl4 taglib-c1v5    # (Para versiones Debian/Ubuntu basadas)
      # - APKPKGS=curl taglib-c         # (Descomenta y usa esta si la imagen usa Alpine)
```

**Si usas LinuxServer (`lscr.io/linuxserver/lidarr`):**
```yaml
    environment:
      # LinuxServer siempre es Alpine Linux
      - DOCKER_MODS=linuxserver/mods:universal-package-install
      - INSTALL_PACKAGES=curl taglib-c
```

Una vez añadido, haz un `docker-compose up -d` para reiniciar Lidarr y que se instalen.

### 4. Activar el Custom Script en Lidarr

1. Entra a la interfaz web de Lidarr.
2. Ve a **Settings → Connect → + → Custom Script**.
3. Rellena los datos:
   - **Name**: `Sync Lyrics`
   - **On Release Import**: Actívalo (✓)
   - **On Upgrade**: Actívalo (✓)
   - **Path**: `/config/scripts/lidarr-lyrics.sh` *(Ojo: esta es la ruta DENTRO del contenedor)*
4. Guarda y dale al botón **Test** para comprobar que funciona.

Opcional: Revisa el archivo de texto `/config/scripts/synclyr2metadata.log` dentro de tu carpeta de configuración para ver el reporte de todo lo que se descarga.

---

## 💻 Uso Manual por Terminal

Si quieres usarlo a mano en tu ordenador o servidor para escanear música que ya tienes, los comandos son muy sencillos.

Ejecútalo pasándole una de estas tres opciones:

### 1. Sincronizar un solo álbum
```bash
./synclyr2metadata --sync "/ruta/a/musica/Artista/Album (2024)"
```

### 2. Sincronizar todos los álbumes de un artista
```bash
./synclyr2metadata --artist "/ruta/a/musica/Lil Wayne" --threads 8
```

### 3. Escanear toda tu biblioteca de golpe
Aviso: asumimos que tu música está organizada en carpetas de `Artistas / Álbumes`.
```bash
./synclyr2metadata --library "/ruta/a/musica" --threads 10
```

### Opciones Extra

| Opción | ¿Para qué sirve? |
|---|---|
| `--force` | Fuerza la descarga sobreescribiendo las letras que ya tuvieras guardadas de antes. |
| `--threads N` | El programa usa descarga en paralelo (multihilo) para ir rapidísimo. Por defecto usa 4 hilos, pero puedes subirlo hasta 16 con `--threads 16`. |
| `--help` | Muestra la ayuda rápida en pantalla. |

### Ejemplo del resultado en pantalla:
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

## Créditos y Licencia

- Letras proveídas por la fantástica base de datos gratuita y abierta [LRCLIB](https://lrclib.net).
- Basado en las librerías open source [libcurl](https://curl.se/), [TagLib](https://taglib.org/) y [cJSON](https://github.com/DaveGamble/cJSON).
- Licencia de código abierto: [GPLv3](LICENSE).