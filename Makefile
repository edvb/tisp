# tisp - tiny lisp
# See LICENSE file for copyright and license details.

include config.mk

ifeq ($(DEBUG), 1)
CFLAGS += -g -Og -DDEBUG
LDFLAGS += -g -Og
endif

EXE = tisp
SRC = tisp.c main.c
TIB = tibs/math.c tibs/io.c tibs/time.c
OBJ = $(SRC:.c=.o) $(TIB:.c=.o)
LIB = tibs/libtibmath.so tibs/libtibio.so
TSP = tibs/lib.tsp tibs/repl.tsp

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

$(OBJ): config.mk

$(LIB): $(TIB)
	@echo $(CC) -o $@
	@$(CC) -shared -o $@ $(OBJ)

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
	@echo installing tsp to $(DESTDIR)$(PREFIX)/bin
	@sed -e "s@\./@@g" -e "s@tibs/@$(DESTDIR)$(PREFIX)/share/tisp/@g" < tsp > $(DESTDIR)$(PREFIX)/bin/tsp
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/tsp
	@echo installing manual page to $(DESTDIR)$(MANPREFIX)/man1
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(EXE).1
	@echo installing shared libraries to $(DESTDIR)$(PREFIX)/lib/tisp
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/tisp
	@cp -f $(LIB) $(DESTDIR)$(PREFIX)/lib/tisp
	@echo installing tisp libraries to $(DESTDIR)$(PREFIX)/share/tisp
	@mkdir -p $(DESTDIR)$(PREFIX)/share/tisp
	@cp -f $(TSP) $(DESTDIR)$(PREFIX)/share/tisp

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
	@$(CC) -o test tisp.o tibs/math.o test.o $(LDFLAGS)
	@./test

man:
	@echo updating man page doc/$(EXE).1
	@markman -nCD -t TISP -V "$(VERSION)" -d "`date '+%B %Y'`" \
		-s "`./$(EXE) -h 2>&1 | cut -d' ' -f2-`" doc/$(EXE).1.md > doc/$(EXE).1

.PHONY: all options clean install uninstall test man
