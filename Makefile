default: all

mkdir:
	mkdir -p bin

all:
	gcc src/main.c -o bin/kcsh

clean:
	rm -rf bin

run:
	./bin/kcsh