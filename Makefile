pull:
	git pull
	sleep 1

compile:
	gcc -pthread flush-reload-basic.c -o flush-reload-basic -falign-functions=128

run:
	./flush-reload-basic

all: pull compile run