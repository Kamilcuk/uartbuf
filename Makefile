# Makefile
src=$(wildcard src/*.c) $(wildcard test/*.c)
inc=$(wildcard src/*.h) $(wildcard test/*.h)
all: test.out
test.out: $(src) $(inc)
	gcc $(src) -fsanitize=address -Wall -lpthread -Isrc -Itest -o $@

test: test.out
	./test.out
clean: 
	rm -f test.out

.PHONY: test clean

