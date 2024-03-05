NAME := $(shell basename $(PWD))
export MODULE := M1
all: $(NAME)-64 $(NAME)-32

include ../Makefile

test1:
	./$(NAME)-64 arg1 arg2 arg3