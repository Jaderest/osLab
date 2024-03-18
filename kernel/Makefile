NAME := kernel
SRCS := kernel.c
export MODULE  := L0
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := x86_64-qemu
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk
image: git

test64: 
	qemu-system-x86_64 -cpu qemu64 -drive format=raw,file=$(PWD)/build/kernel-x86_64-qemu -m 4M

test86: 
	qemu-system-x86 -cpu qemu64 -drive format=raw,file=$(PWD)/build/kernel-x86_64-qemu -m 4M