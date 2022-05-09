pull:
	git pull

attack: attack.c vulnerable.so
	gcc -o attack attack.c -g -lm -lpthread -ldl -l:vulnerable.so -L. -falign-functions=4096

vulnerable.so: vulnerable.c vulnerable.h
	gcc -shared -fpic vulnerable.c -falign-functions=4096 -o vulnerable.so

run-attack: pull attack vulnerable.so
	LD_LIBRARY_PATH=. ./attack

victim: victim.c vulnerable.so
	gcc -o victim victim.c -g -lm -lpthread -ldl -l:vulnerable.so -L.

run-victim: pull victim vulnerable.so
	LD_LIBRARY_PATH=. ./victim

evict: pull
	gcc -o evict evict.c -falign-functions=4096 && ./evict

sleepclear: pull
	gcc -o sleepclear sleepclear.c -falign-functions=4096 && ./sleepclear

i-cross-thread: pull
	gcc -o i-cross-thread i-cross-thread.c -falign-functions=4096 && ./i-cross-thread