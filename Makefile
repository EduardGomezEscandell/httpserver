SOURCES := $(shell find src -name '*.c')
CC ?= gcc

build: main.c $(SOURCES) 
	mkdir -p build
	$(CC) -g -Wall -Wpedantic -Wno-strict-prototypes -Werror main.c $(SOURCES) -o build/server || $(MAKE) clean

.PHONY: clean
clean:
	rm -rf build

.PHONY: run
run: build
	./build/server