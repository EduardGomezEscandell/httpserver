SOURCES := $(shell find src -name '*.c')

build: main.c $(SOURCES) 
	mkdir -p build
	gcc -g -Wall -Wpedantic -Werror main.c $(SOURCES) -o build/server || $(MAKE) clean

.PHONY: clean
clean:
	rm -rf build

.PHONY: run
run: build
	./build/server