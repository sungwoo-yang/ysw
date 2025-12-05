CC := gcc

CFLAGS := -std=c99 -Og -Wall -Wextra -Wpedantic

.PHONY: all
all: getpid out prompt

.PHONY: clean
clean: $(wildcard *.c *.s)
	$(RM) *.o $(patsubst %.s,%,$(^:.c=))
