CC=gcc
CFLAGS=-Wall -Wextra -std=c11
PKG_CONFIG ?= pkg-config
DEP_PACKAGES = libcurl libmicrohttpd

ifeq ($(shell command -v $(PKG_CONFIG) >/dev/null 2>&1 && $(PKG_CONFIG) --exists $(DEP_PACKAGES) && echo yes),yes)
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(DEP_PACKAGES))
LDFLAGS = $(shell $(PKG_CONFIG) --libs $(DEP_PACKAGES))
else
LDFLAGS = -lcurl -lmicrohttpd
endif

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
TARGET=build/server

all: check-deps $(TARGET)

check-deps:
	@printf "Checking dependencies... "
	@if command -v $(PKG_CONFIG) >/dev/null 2>&1 && $(PKG_CONFIG) --exists $(DEP_PACKAGES); then \
		echo "ok"; \
	elif printf '%s\n' '#include <microhttpd.h>' '#include <curl/curl.h>' 'int main(void){return 0;}' | $(CC) -x c - -o /tmp/img-view-depcheck -lcurl -lmicrohttpd >/dev/null 2>&1; then \
		rm -f /tmp/img-view-depcheck; \
		echo "ok"; \
	else \
		echo "missing"; \
		echo "Install required packages, for example on Debian/Ubuntu:"; \
		echo "  sudo apt install libmicrohttpd-dev libcurl4-openssl-dev"; \
		echo "  sudo apt install pkg-config   # optional, for cleaner flags autodetect"; \
		exit 1; \
	fi

$(TARGET): $(OBJ)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean check-deps
