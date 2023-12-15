
scull-objs = scull_ops.o main.o
obj-m += scull.o

CTEST = ./test
KERNELDIR = /lib/modules/$(shell uname -r)/build
CC = gcc

all:
	make -C $(KERNELDIR) M=$(PWD) modules
	make test
	make ins

test:
	$(CC) $(CTEST).c -o $(CTEST)

clean_test:
	rm -rf $(CTEST)

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	make clean_test
	make rm

ins:
	sudo insmod scull.ko
	MAJOR_NUM=$$(sudo dmesg --ctime | tail -1 | grep -Po '\d+(?=(\D+\d+$$))'); \
	sudo mknod /dev/scull1 c $$MAJOR_NUM 0; 
	sudo chmod 777 /dev/scull1
	sudo dmesg --ctime | tail -1
	ls -l /dev | grep scull

rm:
	sudo rmmod scull.ko
	sudo rm -rf /dev/scull1
	sudo dmesg --ctime | tail -1