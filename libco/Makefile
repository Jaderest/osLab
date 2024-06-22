NAME := $(shell basename $(PWD))
export MODULE := M2
all: $(NAME)-64.so $(NAME)-32.so
CFLAGS += -U_FORTIFY_SOURCE

include ../Makefile

commit: git
	git add .
	git commit
	git push