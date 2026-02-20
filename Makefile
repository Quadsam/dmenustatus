CC       := gcc
CPPFLAGS := `pkg-config --cflags-only-I x11 fontconfig alsa`
CFLAGS   := -Wall -Wextra -Werror -O2 -pipe -march=native -fstack-protector-strong -fstack-clash-protection -fcf-protection `pkg-config --cflags-only-other x11 fontconfig alsa`
LDFLAGS  := `pkg-config --libs x11 fontconfig alsa` -lsensors
PROGRAM   = dmenustatus

VERSION   = 0.10.6

DESTDIR  ?=
PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
DOCDIR   ?= $(PREFIX)/share/doc/$(PROGRAM)
MANDIR   ?= $(PREFIX)/share/man

.PHONY: all clean rebuild check dist pkg install uninstall man

all: $(PROGRAM)

clean:
	@echo "  RM	*.o"
	@rm -rf *.o
	@echo "  RM	$(PROGRAM)"
	@rm -rf $(PROGRAM)

rebuild: clean all

check: $(PROGRAM).c
	@cppcheck --enable=all --suppress=missingIncludeSystem --check-level=exhaustive $<

dist:
	@$(MAKE) clean
	@echo "  DIST	$(PROGRAM)-src.tar.gz"
	@mkdir -p $(PROGRAM)-src
	@cp -R Makefile LICENSE README.md dmenustatus.c dmenustatus.1 $(PROGRAM)-src
	@tar -cf $(PROGRAM)-src_v$(VERSION).tar $(PROGRAM)-src
	@gzip -9 $(PROGRAM)-src_v$(VERSION).tar
	@rm -rf $(PROGRAM)-src

pkg:
	@$(MAKE) clean
	@$(MAKE) all
	@$(MAKE) DESTDIR='./pkg' install
	@cd ./pkg && tar cf ../$(PROGRAM)-pkg_v$(VERSION)_x86-64.tar usr
	@gzip -9 $(PROGRAM)-pkg_v$(VERSION)_x86-64.tar
	@rm -rf ./pkg

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

$(PROGRAM).o: $(PROGRAM).c
	@echo "  CC	$@"
	sed -i -r 's|(#define VERSION ")[^"]+|\1'$(VERSION)'|gm' $<
	@${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
