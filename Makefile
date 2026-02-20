CC ?= cc
PKG_CONFIG ?= pkg-config

SOURCES = ./src/main.c ./src/pw.c ./src/ui.c ./src/udp.c ./src/tcp.c ./src/queue.c
PIPEWIRE_FLAGS = $(shell $(PKG_CONFIG) --libs --cflags libpipewire-0.3)

.PHONY: all musl clean

all: awim

awim:
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(PIPEWIRE_FLAGS) -I./include -lpthread $(LDFLAGS)

clean:
	rm -f awim
