ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m += platform-cps100.o
else
# normal makefile
all:	modules

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$$PWD modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$$PWD modules_install
clean:
	rm -f *.o *.ko *.order *.mod* Module.symvers
endif

