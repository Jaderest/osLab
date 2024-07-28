NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c") #这里单独提取了一个main，然后把所有的c文件都加进去
INC_PATH       := include/ framework/                                  #这里把include和framework的路径都加进去，头文件在里面

export MODULE  := L1
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH    := x86_64-qemu
# export ARCH    := native
# export ARCH    := x86-qemu
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk
image: git

.PHONY: test

test:
	@gcc -g $(shell find src/ -name "*.c") \
	$(shell find test/ -name "*.c") \
	-Iframework -Itest -DTEST -lpthread \
	-o build/test

test0: test
	build/test 0

commit:
	git add .
	git commit


all: image, test