.PHONY: format init build test coverage clean

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
	venv/bin/pytest -v


check-all: build venv
	venv/bin/python -m grpc_tools.protoc \
		-I ./proto \
		--python_out=test/ \
		--grpc_python_out=test/ \
		ateles.proto
	venv/bin/pytest -v --include-slow


coverage:
	mkdir -p coverage/
	rm -rf coverage/*
	/Library/Developer/CommandLineTools/usr/bin/llvm-profdata merge -sparse default.profraw -o default.profdata
	/Library/Developer/CommandLineTools/usr/bin/llvm-cov show build/ateles -instr-profile=default.profdata -format=html -output-dir=coverage/
	open coverage/index.html

clean:
	rm -rf build
