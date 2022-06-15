obj-m += read_sxstatus_module.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

test:
	sudo dmesg -C
	sudo insmod read_sxstatus_module.ko
	sudo rmmod read_sxstatus_module.ko
	dmesg