obj-m += scull.o

CTEST = ./test
KERNELDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNELDIR) M=$(PWD) modules
	gcc $(CTEST).c -o $(CTEST)
	make ins

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -rf $(CTEST)
	make rm

ins:
	sudo insmod scull.ko
	LAST_NUM=$$(sudo dmesg --ctime | tail -1 | grep -Po '\d+$$'); \
	PRE_LAST_NUM=$$(sudo dmesg --ctime | tail -1 | grep -Po '\d+(?=(\D+\d+$$))'); \
	sudo mknod /dev/scull c $$PRE_LAST_NUM $$LAST_NUM; \
	sudo chmod 777 /dev/scull
	sudo dmesg --ctime | tail -1

rm:
	sudo rmmod scull.ko
	sudo rm -rf /dev/scull
	sudo dmesg --ctime | tail -1