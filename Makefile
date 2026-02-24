# ─────────────────────────────────────────────────────────────────────────
# synclyr2metadata — Makefile
# ─────────────────────────────────────────────────────────────────────────

CC       := gcc
CFLAGS   := -O2 -Wall -Wextra -pedantic -std=c11 -I/usr/include/taglib
LDFLAGS  := -lcurl -ltag_c -ltag -lz -lpthread

# Directories
SRC_DIR      := src
INC_DIR      := include
THIRD_DIR    := third_party/cjson
BUILD_DIR    := build

# Sources
SRCS := $(SRC_DIR)/main.c \
        $(SRC_DIR)/http_client.c \
        $(SRC_DIR)/lrclib.c \
        $(SRC_DIR)/metadata.c \
        $(THIRD_DIR)/cJSON.c

# Objects (placed in build/)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# Binary
TARGET := synclyr2metadata

# ── Targets ────────────────────────────────────────────────────────────

.PHONY: all clean debug install uninstall

PREFIX   ?= /usr/local

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .c → build/.o (auto-create directories)
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(THIRD_DIR) -c $< -o $@

# Debug build with sanitizers
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
