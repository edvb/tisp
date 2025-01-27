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

all: options $(EXE)

options:
	@echo $(EXE) build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"

tibs.tsp.h: $(TSP)
	@echo xxd $@
	@echo "char tibs[] = { 0x28, " > $@
	@cat $^ | xxd -i - >> $@
	@echo ", 0x29, 0x00};" >> $@

.o:
	@echo $(LD) $@
	@$(LD) -o $@ $< $(LDFLAGS)

.c.o:
	@echo $(CC) $<
	@$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ): $(TIB) tisp.h config.mk

main.o: tibs.tsp.h

test.o: config.mk tibs.tsp.h

$(LIB): $(TIB)
	@echo $(CC) -o $@
	@$(CC) -shared -o $@ $(OBJ)

$(EXE): $(OBJ) $(LIB)
	@echo $(CC) -o $@
	@$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	@echo cleaning
	@rm -f $(OBJ) $(LIB) $(EXE) test test.o tibs.tsp.h

dist: tibs.tsp.h
	@echo creating dist tarball
	@mkdir -p tisp-$(VERSION)
	@cp tisp.h tibs.tsp.h tisp-$(VERSION)
	@sed '/^#include "tib/d' tisp.c > tisp-$(VERSION)/tisp.c
	@cat $(TIB) >> tisp-$(VERSION)/tisp.c
	@tar -cf tisp-$(VERSION).tar tisp-$(VERSION)
	@gzip tisp-$(VERSION).tar
	@rm -rf tisp-$(VERSION)

# TODO don't cp some if not in dynamic mode
install: all
	@echo installing $(EXE) to $(DESTDIR)$(PREFIX)/bin
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@cp -f $(EXE) $(DESTDIR)$(PREFIX)/bin
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo installing tsp to $(DESTDIR)$(PREFIX)/bin
	@sed -e "s@\./@@g" < tsp > $(DESTDIR)$(PREFIX)/bin/tsp
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/tsp
	@echo installing manual page to $(DESTDIR)$(MANPREFIX)/man1
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@cp -f doc/$(EXE).1 $(DESTDIR)$(MANPREFIX)/man1/
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo installing tibs to $(DESTDIR)$(PREFIX)/lib/tisp/pkgs/std
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/tisp/pkgs/std
	@cp -f $(TSP) $(LIB) $(DESTDIR)$(PREFIX)/lib/tisp/pkgs/std

uninstall:
	@echo removing $(EXE) from $(DESTDIR)$(PREFIX)/bin
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo removing manual page from $(DESTDIR)$(MANPREFIX)/man1
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo removing shared libraries from $(DESTDIR)$(PREFIX)/lib/tisp
	@rm -rf $(DESTDIR)$(PREFIX)/lib/tisp/
	@echo removing tisp libraries from $(DESTDIR)$(PREFIX)/share/tisp
	@rm -rf $(DESTDIR)$(PREFIX)/share/tisp/

test: $(OBJ) $(LIB) test.o
	@echo running tests
	@echo $(CC) -o test
	@$(CC) -o test tisp.o test.o $(LDFLAGS)
	@./test

man: $(MAN)

$(MAN): $(DOC) $(EXE)
	@echo updating man page $@
	@markman -nCD -t TISP -V "$(EXE) $(VERSION)" -d "`date '+%B %Y'`" \
		-s "`./$(EXE) -h 2>&1 | cut -d' ' -f2-`" $@.md > $@

.PHONY: all options clean dist install uninstall test man
