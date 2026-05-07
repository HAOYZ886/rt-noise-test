SDKDIR ?= /path/to/sdk/
KERNELDIR ?= $(SDKDIR)/path/to/kernel/

ENV := ARCH=arm64 CROSS_COMPILE=$(SDKDIR)/path/to/toolchain/bin/unknown-linux-gnu-
STRIP := $(CROSS_COMPILE)strip

obj-m += preempt-noise.o 

modules:
	$(MAKE) $(ENV) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) $(ENV) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend *.mod .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order

.PHONY: modules modules_install clean
