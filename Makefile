CC       := gcc
CPPFLAGS +=
CFLAGS   += -Wall -Werror -ggdb -O0
LDFLAGS  += `pkg-config --libs x11 fontconfig`
PROGRAM   = dmenustatus

.PHONY: all clean cleanall test

all: $(PROGRAM)

clean:
	@echo "  RM	*.o"
	@rm -rf *.o
	@echo "  RM	$(PROGRAM)"
	@rm -rf $(PROGRAM)

cleanall:
	@${MAKE} --no-print-directory clean
	@${MAKE} --no-print-directory all

test: $(PROGRAM).c
	@${MAKE} --no-print-directory clean
	splint $^

$(PROGRAM): $(PROGRAM).o
	@echo "  LD	$@"
	@${CC} $(LDFLAGS) -o $@ $^

%.o: %.c
	@echo "  CC	$@"
	@${CC} $(CPPFLAGS) $(CFLAGS) -c -o $@ $^