CC = gcc
CFLAGS = -std=c89 -O3
CPPFLAGS = -D_FILE_OFFSET_BITS=64
LDFLAGS = -lbz2
WD = ./

all: tests
	$(CC) $(CFLAGS) $(CPPFLAGS) $(WD)src/bsdiff.c -o $(WD)bin/bsdiff $(LDFLAGS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(WD)src/bspatch.c -o $(WD)bin/bspatch $(LDFLAGS)

tests:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(WD)src/tests/hw1.c -o $(WD)bin/hw1
	$(CC) $(CFLAGS) $(CPPFLAGS) $(WD)src/tests/hw2.c -o $(WD)bin/hw2

difftest: all tests
	$(WD)bin/bsdiff $(WD)bin/hw1 $(WD)bin/hw2 $(WD)bin/hw2.bsdiff
	$(WD)bin/bspatch $(WD)bin/hw1 $(WD)bin/hw1.patched $(WD)bin/hw2.bsdiff
	chmod +x $(WD)bin/hw1.patched
	$(WD)bin/hw1.patched

clean:
	rm -f $(WD)bin/*
