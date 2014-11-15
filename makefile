CC = gcc
CFLAGS = -std=c89 -O3
CPPFLAGS = -D_FILE_OFFSET_BITS=64
LDFLAGS = -lbz2

all: tests
	$(CC) $(CFLAGS) $(CPPFLAGS) ./src/bsdiff.c -o ./bin/bsdiff $(LDFLAGS)
	$(CC) $(CFLAGS) $(CPPFLAGS) ./src/bspatch.c -o ./bin/bspatch $(LDFLAGS)

tests:
	$(CC) $(CFLAGS) $(CPPFLAGS) ./src/tests/hw1.c -o ./bin/hw1
	$(CC) $(CFLAGS) $(CPPFLAGS) ./src/tests/hw2.c -o ./bin/hw2

difftest: tests
	./bin/bsdiff ./bin/hw1 ./bin/hw2 ./bin/hw2.bsdiff
	./bin/bspatch ./bin/hw1 ./bin/hw1.patched ./bin/hw2.bsdiff
	chmod +x ./bin/hw1.patched
	./bin/hw1.patched

clean:
	rm -f ./bin/*
