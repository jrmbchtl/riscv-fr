compile:
	gcc flush-reload-basic.c -o flush-reload-basic -falign-functions=128

run:
	./flush-reload-basic

all: compile run