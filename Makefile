CC = gcc
CFLAGS = -g
LIBS = -lpthread
all: ringbuf
ringbuf: ringbuf.c
	$(CC) $(CFLAGS) -o ringbuf ringbuf.c $(LIBS)
clean:
	rm -f ringbuf