default: all

mkdir:
	mkdir -p bin

all: mkdir
	gcc main.c -o bin/ksh