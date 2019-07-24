.PHONY: format build test

all: format build


format:
	clang-format -style=file -i src/*


build:
	mkdir -p build
	cmake -S . -B build
	make VERBOSE=1 -C build


clean:
	rm -rf build
