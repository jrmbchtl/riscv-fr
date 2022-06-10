obj-m += read_mcycle_module.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

test:
	sudo dmesg -C
	sudo insmod read_mcycle_module.ko
	sudo rmmod read_mcycle_module.ko
	dmesg