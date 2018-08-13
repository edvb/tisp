# tisp version number
VERSION = 0.0.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# includes and libraries
INCS = -Iinclude
LIBS = -lm -ldl

# flags
CPPFLAGS = -DVERSION=\"$(VERSION)\" -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600
CFLAGS = -g -std=c99 -pedantic -Wall -fPIC $(INCS) $(CPPFLAGS)
LDFLAGS = -g -Wl,-rpath=$(DESTDIR)$(PREFIX)/lib/tisp $(LIBS)

# compiler and linker
CC = cc
