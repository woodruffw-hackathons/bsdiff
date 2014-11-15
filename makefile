CC = gcc
CFLAGS = -std=c89 -O3
CPPFLAGS = -D_FILE_OFFSET_BITS=64
LDFLAGS = -lbz2

all:
	$(CC) $(CFLAGS) $(CPPFLAGS) ./src/bsdiff.c -o ./bin/bsdiff $(LDFLAGS)
	$(CC) $(CFLAGS) $(CPPFLAGS) ./src/bspatch.c -o ./bin/bspatch $(LDFLAGS)

clean:
	rm -f ./bin/*
