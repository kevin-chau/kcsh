default: all

mkdir:
	mkdir -p bin

all:
	gcc main.c -o bin/kcsh

clean:
	rm -rf bin

run:
	./bin/kcsh