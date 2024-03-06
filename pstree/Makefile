NAME := $(shell basename $(PWD))
export MODULE := M1

.PHONY: all test

all: $(NAME)-64 $(NAME)-32

include ../Makefile

commit:
	git add .
	git commit

test: test1 test2

test1:
	./$(NAME)-64 arg1 arg2 arg3

test2:
	./$(NAME)-32 a b c d e f g h