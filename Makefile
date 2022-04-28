pull:
	git pull
	sleep 1

compile:
	gcc flush-reload-basic.c -o flush-reload-basic -falign-functions=8193

run:
	./flush-reload-basic

all: pull compile run