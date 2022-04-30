pull:
	git pull
	sleep 1

attack: attack-me.so
	gcc -o attack attack.c -g -lm -lpthread -ldl -L.

attack-me.so: attack-me.c attack-me.h
	gcc -shared -fpic attack-me.c -falign-functions=4096 -o attack-me.so

run: attack attack-me.so
	LD_LIBRARY_PATH=. ./attack

all: pull compile run