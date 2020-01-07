.PHONY: all

COMPILE=gcc -c
LINK=gcc

WRMATH=../math
WRDSP=../dsp

INCLUDES = -I$(WRMATH)/ -I$(WRDSP)/

SRCS = $(wildcard math/*.c)
SRCS = $(wildcard dsp/*.c)

OPTIMIZE = -O2

CFLAGS=-I. $(INCLUDES) -DPROFILE $(OPTIMIZE)

all: $(SRCS)
	@gcc $(CFLAGS) -g -c dsp/Profile_wrLpGate.c ../dsp/wrLpGate.c
	@gcc -o myprog Profile_wrLpGate.o wrLpGate.o
	@./myprog
