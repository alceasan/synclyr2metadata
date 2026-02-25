# ── synclyr2metadata Makefile ──────────────────────────────────────────
#
# Default build compiles inside an Alpine Docker container so the
# resulting binary is compatible with Lidarr/Navidrome Docker images.
#
#   make            → Alpine build via Docker (recommended)
#   make native     → Local build (needs libcurl-dev, taglib-dev)
#   make install    → Install to /usr/local/bin
#   make clean      → Remove build artifacts
#

CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -pedantic -std=c11
TARGET   = synclyr2metadata

SRC_DIR   = src
INC_DIR   = include
THIRD_DIR = third_party/cjson
BUILD_DIR = build

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/http_client.c \
       $(SRC_DIR)/lrclib.c \
       $(SRC_DIR)/metadata.c \
       $(THIRD_DIR)/cJSON.c

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
LDFLAGS = -lcurl -ltag_c -ltag -lz -lpthread

PREFIX ?= /usr/local

# ── Targets ────────────────────────────────────────────────────────────

.PHONY: all native clean debug install uninstall

# Default: build inside Alpine Docker (portable, works in Lidarr/Docker)
all:
	docker run --rm -v "$$(pwd)":/build -w /build alpine:3.22 sh -c " \
		apk add build-base curl-dev taglib-dev && \
		make native"

# Local build (requires libcurl-dev + taglib-dev on your system)
native: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(THIRD_DIR) -c $< -o $@

debug: CFLAGS := -g -O0 -Wall -Wextra -pedantic -std=c11 -fsanitize=address,undefined
debug: LDFLAGS += -fsanitize=address,undefined
debug: clean $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
