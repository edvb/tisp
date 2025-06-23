# tisp - tiny lisp
# See LICENSE file for copyright and license details.

include config.mk

ifeq ($(DEBUG), 1)
CFLAGS += -g -Og
LDFLAGS += -g -Og
endif

EXE = tisp
SRC = tisp.c main.c
TIB = tib/core.c tib/string.c tib/math.c tib/io.c tib/os.c
OBJ = $(SRC:.c=.o)
LIB = $(TIB:.c=.so)
TSP = tib/core.tsp tib/list.tsp tib/doc.tsp tib/io.tsp tib/math.tsp tib/os.tsp
DOC = doc/tisp.1.md doc/tisp.7.md
MAN = $(DOC:.md=)

MANOPTS = -nCD -t TISP -V "$(EXE) $(VERSION)" -d "`date '+%B %Y'`"
VERSIONSHORT=$(shell cut -d '.' -f 1,2 <<< $(VERSION))

all: options $(EXE)

options:
	@echo $(EXE) build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"

tibs.tsp.h: $(TSP)
	@echo xxd $@
	@echo "char tibs[] = { " > $@
	@cat $^ | xxd -i - >> $@
	@echo ", 0x00};" >> $@

.o:
	@echo $(LD) $@
	@$(LD) -o $@ $< $(LDFLAGS)

.c.o:
	@echo $(CC) $<
	@$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ): $(TIB) tisp.h config.mk

main.o: tibs.tsp.h

$(LIB): $(TIB)
	@echo $(CC) -o $@
	@$(CC) -shared -o $@ $(OBJ)

$(EXE): $(OBJ) $(LIB)
	@echo $(CC) -o $@
	@$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	@echo cleaning
	@rm -f $(OBJ) $(LIB) $(EXE) test/test test/test.o tibs.tsp.h

man: $(MAN)

%.1: %.1.md $(EXE)
	@echo updating man page $@
	@markman $(MANOPTS) -s "`./$(EXE) -h 2>&1 | cut -d' ' -f2-`" $< > $@

%.7: %.7.md $(EXE)
	@echo updating man page $@
	@markman $(MANOPTS) -7 $< > $@

dist: tibs.tsp.h
	@echo creating dist tarball
	@mkdir -p tisp-$(VERSION)
	@cp tisp.h tibs.tsp.h tisp-$(VERSION)
	@sed '/^#include "tib/d' tisp.c > tisp-$(VERSION)/tisp.c
	@cat $(TIB) >> tisp-$(VERSION)/tisp.c
	@tar -cf tisp-$(VERSION).tar tisp-$(VERSION)
	@gzip tisp-$(VERSION).tar
	@rm -rf tisp-$(VERSION)

install: all
	@echo installing $(DESTDIR)$(PREFIX)/bin/$(EXE)$(VERSIONSHORT)
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@cp -f $(EXE) $(DESTDIR)$(PREFIX)/bin/$(EXE)$(VERSIONSHORT)
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/$(EXE)$(VERSIONSHORT)
	@echo installing $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@ln -sf $(EXE)$(VERSIONSHORT) $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo installing $(DESTDIR)$(PREFIX)/bin/tsp
	@sed -e "s@\./@@g" < tsp > $(DESTDIR)$(PREFIX)/bin/tsp
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/tsp
	@echo installing $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo installing $(DESTDIR)$(MANPREFIX)/man7/$(EXE).7
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man7
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@cp -f doc/$(EXE).1 $(DESTDIR)$(MANPREFIX)/man1/
	@cp -f doc/$(EXE).7 $(DESTDIR)$(MANPREFIX)/man7/
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man7/$(EXE).7
	@echo installing tibs to $(DESTDIR)$(PREFIX)/lib/tisp/pkgs/std
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/tisp/pkgs/std
	@cp -f $(TSP) $(LIB) $(DESTDIR)$(PREFIX)/lib/tisp/pkgs/std

uninstall:
	@echo removing $(EXE) from $(DESTDIR)$(PREFIX)/bin
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo removing manual page from $(DESTDIR)$(MANPREFIX)/man1
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo removing manual page from $(DESTDIR)$(MANPREFIX)/man7
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/$(EXE).7
	@echo removing shared libraries from $(DESTDIR)$(PREFIX)/lib/tisp
	@rm -rf $(DESTDIR)$(PREFIX)/lib/tisp/
	@echo removing tisp libraries from $(DESTDIR)$(PREFIX)/share/tisp
	@rm -rf $(DESTDIR)$(PREFIX)/share/tisp/

test: $(OBJ) $(LIB) test/tests.h test/test.o
	@echo running tests
	@echo $(CC) -o test/test
	@$(CC) -o test/test tisp.o test/test.o $(LDFLAGS)
	@./test/test

.PHONY: all options clean man dist install uninstall test
