t-objs=testmod.o kmemscratch.o
obj-m=t.o

KERNELPATH=/usr/src/linux-headers-$(shell uname -r)

all:
	make -C ${KERNELPATH} M=${CURDIR} modules

clean:
	make -C ${KERNELPATH} M=${CURDIR} clean

