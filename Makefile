.PHONY: format init build test clean

all: init build


format:
	clang-format -style=file -i src/*


init:
	test -d build || (mkdir build && cmake -S . -B build)


build:
	make -C build


clean:
	rm -rf build
