default: all

mkdir:
	mkdir -p bin

all:
	gcc main.c -o bin/ksh

clean:
	rm -rf bin