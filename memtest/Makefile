.PHONY: all

COMPILE=gcc -c
LINK=gcc

WRMATH=../math
WRDSP=../dsp

INCLUDES = -I$(WRMATH)/ -I$(WRDSP)/

SRCS += $(WRMATH)/wrMath.c
SRCS += $(WRMATH)/wrQueue.c
SRCS += $(WRDSP)/wrBlocks.c
SRCS += $(WRDSP)/wrBuffer.c
SRCS += $(WRDSP)/wrBufferInterface.c
SRCS += $(WRDSP)/wrClip.c
SRCS += $(WRDSP)/wrDelay.c
SRCS += $(WRDSP)/wrFilter.c
SRCS += $(WRDSP)/wrFuncGen.c
SRCS += $(WRDSP)/wrHistory.c
SRCS += $(WRDSP)/wrIHead.c
SRCS += $(WRDSP)/wrInterpolate.c
SRCS += $(WRDSP)/wrIPlayer.c
SRCS += $(WRDSP)/wrLpGate.c
SRCS += $(WRDSP)/wrOscSine.c
SRCS += $(WRDSP)/wrTransport.c
SRCS += $(WRDSP)/wrVtl.c

OPTIMIZE = -O0

CFLAGS=-I. $(INCLUDES) -DPROFILE $(OPTIMIZE)

all: $(SRCS)
	@gcc $(CFLAGS) memtest.c $(SRCS) -o myprog -lm
	valgrind --leak-check=yes ./myprog
