NAME=$(notdir $(CURDIR))
VERSION=0.1-dev

src = $(wildcard src/*.c)
obj = $(src:.c=.o)

CFLAGS=-std=c99 -pedantic -Wshadow -Wstrict-aliasing -Wstrict-overflow \
	   -Wextra -Wall
DEBUGFLAGS=-Og -g -ggdb
RELEASEFLAGS=-O3 -s
LIBS=-lncurses

CROSSCC=zig cc -DUNITY_BUILD

debug: $(obj)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LIBS) -o $(NAME) $^

release: $(obj)
	$(CC) $(CFLAGS) -march=native $(RELEASEFLAGS) $(LIBS) -o $(NAME) $^

cross: src/main.c
	mkdir -p release
	$(CROSSCC) -static -target x86_64-windows     $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-x86_64-win.exe  $^
	$(CROSSCC) -static -target aarch64-windows    $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-aarch64-win.exe $^
	$(CROSSCC) -static -target x86_64-linux-musl  $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-x86_64-linux    $^
	$(CROSSCC) -static -target aarch64-linux-musl $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-aarch64-linux   $^
	$(CROSSCC) -static -target x86_64-macos       $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-x86_64-macos    $^
	$(CROSSCC) -static -target aarch64-macos      $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-aarch64-macos   $^

copy:
	cp $(NAME) ~/.local/bin/

install: release copy

.PHONY: clean
clean:
	rm -f $(obj) debug
