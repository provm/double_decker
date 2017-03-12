obj-m := cgtmem.o
cgtmem-objs := tmem.o utmem.o tcache.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

