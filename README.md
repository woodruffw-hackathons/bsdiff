bsdiff
=======

A fork of Colin Percival's binary diff utilities, cleaned up and modernized for use during HackDuke 2014.

## Dependencies:
Like the original `bsdiff`, this fork relies on bzip2 to compress its diffs.

## Changes:
* Replaced unportable `u_char` declarations with `uint8_t` types.
* Simplified the makefile.
* Fixed formatting.
* Replaced BSD-style `err` functions with `fprintf()`/`exit()` combinations.
