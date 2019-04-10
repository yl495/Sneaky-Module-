ifeq ($(KERNELRELEASE),)  

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 
PWD := $(shell pwd)  

.PHONY: build clean  

build: sneaky_process
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

sneaky_process: sneaky_process.o
	gcc -o sneaky_process sneaky_process.o

sneaky_process.o: sneaky_process.c
	gcc -std=gnu99 -pedantic -Wall -c sneaky_process.c

clean:
	rm -rf *.o *~ core .depend .*.cmd *.order *.symvers *.ko *.mod.c 
else  

$(info Building with KERNELRELEASE = ${KERNELRELEASE}) 
obj-m :=    sneaky_mod.o  

endif
