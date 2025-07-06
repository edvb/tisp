# eevo - easy expression value organizer
# See LICENSE file for copyright and license details.

include config.mk

ifeq ($(DEBUG), 1)
CFLAGS += -g -Og
LDFLAGS += -g -Og
endif

EXE = eevo
SRC = eevo.c main.c
OBJ = $(SRC:.c=.o)
LIB = $(CORE:.c=.so)
DOC = doc/eevo.1.md doc/eevo.7.md
MAN = $(DOC:.md=)

MANOPTS = -nCD -t EEVO -V "$(EXE) $(VERSION)" -d "`date '+%B %Y'`"
VERSIONSHORT=$(shell cut -d '.' -f 1,2 <<< $(VERSION))

all: options $(EXE)

options:
	@echo $(EXE) build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"

core.evo.h: $(EVO)
	@echo xxd $@
	@echo "char eevo_core[] = { " > $@
	@cat $^ | xxd -i - >> $@
	@echo ", 0x00};" >> $@

.o:
	@echo $(LD) $@
	@$(LD) -o $@ $< $(LDFLAGS)

.c.o:
	@echo $(CC) $<
	@$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ): $(CORE) eevo.h config.mk

main.o: core.evo.h

$(LIB): $(CORE)
	@echo $(CC) -o $@
	@$(CC) -shared -o $@ $(OBJ)

$(EXE): $(OBJ) $(LIB)
	@echo $(CC) -o $@
	@$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	@echo cleaning
	@rm -f $(OBJ) $(LIB) $(EXE) test/test test/test.o core.evo.h

man: $(MAN)

%.1: %.1.md $(EXE)
	@echo updating man page $@
	@markman $(MANOPTS) -s "`./$(EXE) -h 2>&1 | cut -d' ' -f2-`" $< > $@

%.7: %.7.md $(EXE)
	@echo updating man page $@
	@markman $(MANOPTS) -7 $< > $@

dist: core.evo.h
	@echo creating dist tarball
	@mkdir -p eevo-$(VERSION)
	@cp eevo.h core.evo.h eevo-$(VERSION)
	@sed '/^#include "core/d' eevo.c > eevo-$(VERSION)/eevo.c
	@cat $(CORE) >> eevo-$(VERSION)/eevo.c
	@tar -cf eevo-$(VERSION).tar eevo-$(VERSION)
	@gzip eevo-$(VERSION).tar
	@rm -rf eevo-$(VERSION)

install: all
	@echo installing $(DESTDIR)$(PREFIX)/bin/$(EXE)$(VERSIONSHORT)
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@cp -f $(EXE) $(DESTDIR)$(PREFIX)/bin/$(EXE)$(VERSIONSHORT)
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/$(EXE)$(VERSIONSHORT)
	@echo installing $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@ln -sf $(EXE)$(VERSIONSHORT) $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo installing $(DESTDIR)$(PREFIX)/bin/evo
	@sed -e "s@\./@@g" < evo > $(DESTDIR)$(PREFIX)/bin/evo
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/evo
	@echo installing $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo installing $(DESTDIR)$(MANPREFIX)/man7/$(EXE).7
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man7
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@cp -f doc/$(EXE).1 $(DESTDIR)$(MANPREFIX)/man1/
	@cp -f doc/$(EXE).7 $(DESTDIR)$(MANPREFIX)/man7/
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man7/$(EXE).7
	@echo installing core to $(DESTDIR)$(PREFIX)/lib/eevo/pkgs/core
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/eevo/pkgs/core
	@cp -f $(EVO) $(LIB) $(DESTDIR)$(PREFIX)/lib/eevo/pkgs/core

uninstall:
	@echo removing $(EXE) from $(DESTDIR)$(PREFIX)/bin
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo removing manual page from $(DESTDIR)$(MANPREFIX)/man1
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo removing manual page from $(DESTDIR)$(MANPREFIX)/man7
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/$(EXE).7
	@echo removing shared libraries from $(DESTDIR)$(PREFIX)/lib/eevo
	@rm -rf $(DESTDIR)$(PREFIX)/lib/eevo/
	@echo removing eevo libraries from $(DESTDIR)$(PREFIX)/share/eevo
	@rm -rf $(DESTDIR)$(PREFIX)/share/eevo/

test: $(OBJ) $(LIB) test/tests.h test/test.o
	@echo running tests
	@echo $(CC) -o test/test
	@$(CC) -o test/test eevo.o test/test.o $(LDFLAGS)
	@./test/test

.PHONY: all options clean man dist install uninstall test
