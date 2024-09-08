SOURCES := $(shell find src -name '*.c')
CC ?= gcc
FLAGS ?= -g -O0 -Wall -Wpedantic -Werror -Wno-strict-prototypes
SANITIZE ?= -fsanitize=address -fsanitize=undefined

build: main.c $(SOURCES) 
	mkdir -p build
	$(CC) $(FLAGS) $(SANITIZE) main.c $(SOURCES) -o build/server || ($(MAKE) clean && false)

.PHONY: clean
clean:
	rm -rf build

.PHONY: run
run: build
	./build/server