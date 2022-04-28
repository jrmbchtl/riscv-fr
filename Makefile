pull:
	git pull
	sleep 1

compile:
	gcc flush-reload-basic.c -o flush-reload-basic -falign-functions=8192

run:
	./flush-reload-basic

all: pull compile run