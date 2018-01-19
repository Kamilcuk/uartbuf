# Makefile
src=$(wildcard src/*.c) $(wildcard test/*.c)
inc=$(wildcard src/*.h) $(wildcard test/*.h)
all: test.out
test.out: $(src) $(inc)
	gcc -fsanitize=address -std=gnu11 -lm -Wall -lpthread -Itest -Isrc -g3 -O3 $(src) -o $@

test: test.out
	./test.out
clean: 
	rm -f test.out

.PHONY: test clean

