#COMPILER ?= mpicc
COMPILER ?= mpicc

CFLAGS ?= -O2 -g -Wall -Wno-variadic-macros -pedantic -DDEBUG $(GCC_SUPPFLAGS)
LDFLAGS ?= -g 
LDLIBS =

EXECUTABLE = player/random

SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:src/%.c=player/%.o)

random: src/random.c
	mpicc -o player/random src/random.c

last: src/v1.31.c
	mpicc -o player/last src/v1.31.c

me: src/v1.4.1.c
	mpicc -o player/latest src/v1.4.1.c

all: release

release: $(OBJS)
	$(COMPILER) $(LDFLAGS) -o $(EXECUTABLE) $(OBJS) $(LDLIBS) 

player/%.o: src/%.c
	$(COMPILER) $(CFLAGS) -o $@ -c $<

clean:
	rm -f player/*.o
	rm ${EXECUTABLE} 

cleandata:
	rm white*.txt
	rm black*.txt
	rm -r Logs/*
