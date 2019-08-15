# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

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
