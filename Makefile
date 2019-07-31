.PHONY: format init build test clean

all: build


format:
	clang-format -style=file -i src/*


init:
	test -d build || (mkdir build && cmake -S . -B build)


build: init
	make -C build


venv:
	virtualenv venv
	venv/bin/pip install -r test/requirements.txt


check: build venv
	venv/bin/python -m grpc_tools.protoc \
		-I ./proto \
		--python_out=test/ \
		--grpc_python_out=test/ \
		ateles.proto
	venv/bin/pytest


clean:
	rm -rf build
