NAME := kernel
SRCS := kernel.c
export MODULE  := L0
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := x86_64-qemu
# export ARCH := x86-qemu
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk
image: git

commit:
	git add .
	git commit