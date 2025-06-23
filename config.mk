# tisp version number
VERSION = 0.1

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
LIBPREFIX = $(PREFIX)/lib/tisp/pkgs/std

# includes and libraries
INCS = -Iinclude
LIBS = -lm -ldl

# flags
DEFINES = -DVERSION=\"$(VERSION)\" -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -O3 -pedantic -Wall -fPIC $(INCS) $(DEFINES)
LDFLAGS = -O3 -Wl,-rpath=$(DESTDIR)$(PREFIX)/lib/tisp $(LIBS)

# turn off debug mode by default
DEBUG ?= 0

# do not load tibs statically, use load procedure instead
# TODO include -ldl when on, enable (load) of shared libs
# CFLAGS += -DTIB_DYNAMIC

# compiler and linker
CC ?= cc
