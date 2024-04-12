NAME := $(shell basename $(PWD))
export MODULE := M2
all: $(NAME)-64.so $(NAME)-32.so
CFLAGS += -U_FORTIFY_SOURCE
CFLAGS += -DLOCAL_MACHINE

include ../Makefile

commit:
	git add .
	git commit
	git push