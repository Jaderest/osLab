NAME := $(shell basename $(PWD))
export MODULE := M5
all: $(NAME)-64 $(NAME)-32

include ../Makefile

fast: $(NAME)-64
	./$(NAME)-64 find .

ez: $(NAME)-32
	./$(NAME)-32 true

hd1: $(NAME)-64
	./$(NAME)-64 find /