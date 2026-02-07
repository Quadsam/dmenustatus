CC       := gcc
CPPFLAGS += `pkg-config --cflags-only-I x11 fontconfig`
CFLAGS   += -Wall -Werror -ggdb -O0 `pkg-config --cflags-only-other x11 fontconfig`
LDFLAGS  += `pkg-config --libs x11 fontconfig`
PROGRAM   = dmenustatus


DESTDIR  ?=
PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
DOCDIR   ?= $(PREFIX)/share/doc/$(PROGRAM)
MANDIR   ?= $(PREFIX)/share/man

.PHONY: all clean rebuild test dist install uninstall man

all: $(PROGRAM)

clean:
	@echo "  RM	*.o"
	@rm -rf *.o
	@echo "  RM	$(PROGRAM)"
	@rm -rf $(PROGRAM)

rebuild: clean all

test: $(PROGRAM).c
	@${MAKE} --no-print-directory clean
	splint $<

dist: clean
	@echo "  DIST	$(PROGRAM)-dist.tar.gz"
	@mkdir -p $(PROGRAM)-dist
	@cp -R Makefile LICENSE README.md dmenustatus.c dmenustatus.1 $(PROGRAM)-dist
	@tar -cf $(PROGRAM)-dist.tar $(PROGRAM)-dist
	@gzip -9 $(PROGRAM)-dist.tar
	@rm -rf $(PROGRAM)-dist

install: all
	@echo "  I	bin $(PROGRAM) to $(DESTDIR)$(BINDIR)"
	@install -Dm755 $(PROGRAM) $(DESTDIR)$(BINDIR)/$(PROGRAM)
	@echo "  I	doc to $(DESTDIR)$(DOCDIR)"
	@install -Dm644 README.md $(DESTDIR)$(DOCDIR)/README.md
	@install -Dm644 LICENSE $(DESTDIR)$(DOCDIR)/LICENSE
	@echo "  I	man to $(DESTDIR)$(MANDIR)/man1"
	@install -Dm644 dmenustatus.1 $(DESTDIR)$(MANDIR)/man1/$(PROGRAM).1

uninstall:
	@echo "  RM	$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	@rm -rf $(DESTDIR)$(BINDIR)/$(PROGRAM)
	@echo "  RM	$(DESTDIR)$(DOCDIR)"
	@rm -rf $(DESTDIR)$(DOCDIR)
	@echo "  RM	$(DESTDIR)$(MANDIR)/man1/$(PROGRAM).1"
	@rm -rf $(DESTDIR)$(MANDIR)/man1/$(PROGRAM).1

$(PROGRAM): $(PROGRAM).o
	@echo "  LD	$@"
	@${CC} -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "  CC	$@"
	@${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
