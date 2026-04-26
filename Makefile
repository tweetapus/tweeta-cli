CC ?= cc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LIBCURL_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcurl 2>/dev/null)
LIBCURL_LIBS := $(shell $(PKG_CONFIG) --libs libcurl 2>/dev/null || echo -lcurl)
SRC := src/main.c src/config.c src/util.c src/http.c src/routes.c src/commands.c src/jsonfmt.c src/cJSON.c

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

all: tweeta

tweeta: $(SRC) src/tweeta.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIBCURL_CFLAGS) -o $@ $(SRC) $(LIBCURL_LIBS)

install: tweeta
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 tweeta "$(DESTDIR)$(BINDIR)/tweeta"

clean:
	rm -f tweeta

.PHONY: all install clean
