CC=gcc
CCOPTS=-Wall -Wextra -g -pthread
LIBS=

SRCS=$(wildcard *.c)
TARGETS=$(SRCS:.c=)

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

%: %.c
	$(CC) $(CCOPTS) -o $@ $< $(LIBS)

start_p: build
	./build/Knight_Concurrency -p -n $(N) -c $(C)

start_d: build
	./build/Knight_Concurrency -d

start_b: build
	./build/Knight_Concurrency -b

start_b_proc: build
	./build/Knight_Concurrency -proc -b

build: concurrency_problems.c
	@$(CC) --std=gnu99 -Wall -pthread -o ./build/Knight_Concurrency councurrency_problems.c

debug: concurrecny_problems.c
	@$(CC) --std=gnu99 -g -pthread -o ./build/Knight_Concurrency concurrency_progblems.c





