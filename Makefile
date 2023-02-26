NAME=$(notdir $(CURDIR))
VERSION=0.2

src = $(wildcard src/*.c)
obj = $(src:.c=.o)

CFLAGS=-std=c99 -pedantic -Wshadow -Wstrict-aliasing -Wstrict-overflow \
	   -Wextra -Wall
DEBUGFLAGS=-Og -g -ggdb
RELEASEFLAGS=-O3 -s
LIBS=-lncurses

debug: $(obj)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LIBS) -o $(NAME) $^

release: $(obj)
	$(CC) $(CFLAGS) -march=native $(RELEASEFLAGS) $(LIBS) -o $(NAME) $^

copy:
	cp $(NAME) ~/.local/bin/

install: release copy

.PHONY: clean
clean:
	rm -f $(obj) debug
