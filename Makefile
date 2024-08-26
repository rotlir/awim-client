all:
	$(CC) $(CFLAGS) -fdiagnostics-color=always -g ./src/main.c ./src/pw.c ./src/ui.c ./src/udp.c ./src/tcp.c ./src/queue.c -o awim `pkg-config --libs --cflags libpipewire-0.3` -I./include -lpthread