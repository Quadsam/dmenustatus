CC       := gcc
CPPFLAGS += `pkg-config --cflags-only-I x11 fontconfig`
CFLAGS   += -Wall -Werror -ggdb -O0 `pkg-config --cflags-only-other x11 fontconfig`
LDFLAGS  += `pkg-config --libs x11 fontconfig`
PROGRAM   = dmenustatus
DESTDIR  ?=
PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin

.PHONY: all clean rebuild test install

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

install: all
	@echo "  I	$(PROGRAM) to $(DESTDIR)$(BINDIR)"
	@install -Dm755 $(PROGRAM) $(DESTDIR)$(BINDIR)/$(PROGRAM)

uninstall:
	@echo "  RM	$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	@rm -rf $(DESTDIR)$(BINDIR)/$(PROGRAM)

$(PROGRAM): $(PROGRAM).o
	@echo "  LD	$@"
	@${CC} -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "  CC	$@"
	@${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $<