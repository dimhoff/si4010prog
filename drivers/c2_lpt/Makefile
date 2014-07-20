#Kernel module makefile for 2.4.xx kernel
CC=gcc
#The line below should consider the location of kernel headers
# (/usr/src/kernel-headers-2.4.25/include in my case)
MODCFLAGS := -O2 -I /usr/src/kernel-headers-2.4.25/include -Wall -DMODULE -D__KERNEL__ -DLINUX
all:	cygnal_c2.o
cygnal_c2.o: cygnal_c2.c c2_ioctls.h c2_lpt.h
	$(CC) $(MODCFLAGS) -c cygnal_c2.c
