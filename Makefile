# tisp - tiny lisp
# See LICENSE file for copyright and license details.

include config.mk

EXE = tisp
SRC = tisp.c main.c
TIB = tibs/math.c tibs/io.c
OBJ = $(SRC:.c=.o) $(TIB:.c=.o)
LIB = tibs/libtibmath.so tibs/libtibio.so

all: options $(EXE)

options:
	@echo $(EXE) build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"

.o:
	@echo $(LD) $@
	@$(LD) -o $@ $< $(LDFLAGS)

.c.o:
	@echo $(CC) $<
	@$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ): config.h config.mk

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

$(LIB): $(TIB)
	@echo $(CC) -o $@
	@gcc -shared -o $@ $(OBJ)

$(EXE): $(OBJ) $(LIB)
	@echo $(CC) -o $@
	@$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	@echo cleaning
	@rm -f $(OBJ) $(LIB) $(EXE) test test.o test.out

install: all
	@echo installing $(EXE) to $(DESTDIR)$(PREFIX)/bin
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@cp -f $(EXE) $(DESTDIR)$(PREFIX)/bin
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo installing manual page to $(DESTDIR)$(MANPREFIX)/man1
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo installing libraries to $(DESTDIR)$(PREFIX)/lib/tisp
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/tisp
	@cp -f $(LIB) $(DESTDIR)$(PREFIX)/lib/tisp

uninstall:
	@echo removing $(EXE) from $(DESTDIR)$(PREFIX)/bin
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(EXE)
	@echo removing manual page from $(DESTDIR)$(MANPREFIX)/man1
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo removing libraries from $(DESTDIR)$(PREFIX)/lib/tisp
	@rm -rf $(DESTDIR)$(PREFIX)/lib/tisp/

test: $(OBJ) $(LIB) test.o
	@echo running tests
	@echo $(CC) -o test
	@$(CC) -o test tisp.o tibs/math.o test.o $(LDFLAGS)
	@./test

man:
	@echo updating man page doc/$(EXE).1
	@markman -nCD -t TISP -V "$(VERSION)" -d "`date '+%B %Y'`" \
		-s "`./$(EXE) -h 2>&1 | cut -d' ' -f2-`" doc/$(EXE).1.md > doc/$(EXE).1

.PHONY: all options clean install uninstall test man
