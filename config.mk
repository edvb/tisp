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
CFLAGS = -std=c99 -pedantic -Wall -fPIC $(INCS) $(CPPFLAGS)
LDFLAGS = -Wl,-rpath=$(DESTDIR)$(PREFIX)/lib/tisp $(LIBS)

# turn off debug mode by default
DEBUG ?= 0

# self evaluate symbols if they are not defined, instead of throwing error
# CFLAGS += -DTSP_SYM_RETURN

# do not load tibs statically, use load procedure instead
# CFLAGS += -DTIB_DYNAMIC

# compiler and linker
CC = cc
