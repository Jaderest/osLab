NAME := $(shell basename $(PWD))
export MODULE := M4
all: $(NAME)-64 $(NAME)-32
LDFLAGS += -ldl
CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-label -Wno-unused-result

include ../Makefile

run64: $(NAME)-64
	./$<

run32: $(NAME)-32
	./$<