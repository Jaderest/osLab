NAME := $(shell basename $(PWD))
export MODULE := M4
all: $(NAME)-64 $(NAME)-32
LDFLAGS += -ldl

include ../Makefile

run64: $(NAME)-64
	./$<

run32: $(NAME)-32
	./$<