obj-m += hello.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

ins:
	sudo insmod hello.ko
	sudo dmesg --ctime | tail -1

rm:
	sudo rmmod hello.ko
	sudo dmesg --ctime | tail -1
