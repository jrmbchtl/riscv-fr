pull:
	git pull
	sleep 1

attack: attack.c vulnerable.so
	gcc -o attack attack.c -g -lm -lpthread -ldl -lvulnerable -L.

vulnerable.so: vulnerable.c vulnerable.h
	gcc -shared -fpic vulnerable.c -falign-functions=4096 -o vulnerable.so

run: attack vulnerable.so
	LD_LIBRARY_PATH=. ./attack

all: pull compile run