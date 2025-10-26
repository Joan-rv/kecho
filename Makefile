obj-m += kecho.o

PWD := $(CURDIR)
OUTDIR := $(PWD)/build

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) MO=$(OUTDIR) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) MO=$(OUTDIR) clean
