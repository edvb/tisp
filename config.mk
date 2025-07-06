# eevo version number
VERSION = 0.1

# core modules to include
CORE = core/core.c core/string.c core/math.c core/io.c core/os.c
EVO = core/core.evo core/list.evo core/doc.evo core/io.evo core/math.evo core/os.evo

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
LIBPREFIX = $(PREFIX)/lib/eevo/pkgs/std

# includes and libraries
INCS = -Iinclude
LIBS = -lm -ldl

# flags
DEFINES = -DVERSION=\"$(VERSION)\" -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -O3 -pedantic -Wall -fPIC $(INCS) $(DEFINES)
LDFLAGS = -O3 -Wl,-rpath=$(DESTDIR)$(PREFIX)/lib/eevo $(LIBS)

# turn off debug mode by default
DEBUG ?= 0

# compiler and linker
CC ?= cc
