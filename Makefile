obj-m := vwire_module.o
vwire_module-objs := vwire_main.o vwire.o 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
