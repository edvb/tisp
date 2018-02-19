# tisp version number
VERSION = 0.0.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# includes and libraries
INCS = -Iinclude
LIBS = -lm

# flags
CPPFLAGS = -DVERSION=\"$(VERSION)\" -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600
CFLAGS = -g -std=c99 -pedantic -Wall $(INCS) $(CPPFLAGS)
LDFLAGS = -g $(LIBS)

# compiler and linker
CC = cc
