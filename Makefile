pull:
	git pull
	sleep 1

attack:
	gcc -o attack attack.c -g -lm -lpthread -ldl -lenc -L.

attack-me.so: attack-me.c attack-me.h
	gcc -shared -fpic attack-me.c -falign-functions=4096 -o attack-me.so

run:
	LD_LIBRARY_PATH=. ./attack

all: pull compile run