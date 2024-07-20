NAME := $(shell basename $(PWD))
export MODULE := M6
all: $(NAME)-64 $(NAME)-32
LDFLAGS += -lm

include ../Makefile

show: $(NAME)-64
	./fsrecov-64 fsrecov.img
