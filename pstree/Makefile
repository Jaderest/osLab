NAME := $(shell basename $(PWD))
export MODULE := M1

.PHONY: all test

all: $(NAME)-64 $(NAME)-32

include ../Makefile

commit:
	git add .
	git commit

test: test1 test2 test3 test4 test5 test6

test1:
	./$(NAME)-64

test2:
	./$(NAME)-64 -n -V -p

test3:
	./$(NAME)-64 -n
	./$(NAME)-64 -V
	./$(NAME)-64 -p

test4:
	./$(NAME)-32

test5:
	./$(NAME)-32 -n -V -p

test6:
	./$(NAME)-32 -n
	./$(NAME)-32 -V
	./$(NAME)-32 -p
