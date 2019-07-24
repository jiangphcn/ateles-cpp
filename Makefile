#TEST_FILES = $(filter-out tests/runner.cc, $(wildcard tests/*.cc))
CXX         = clang++
CC          = clang
CPP         = clang
LINK        = clang++
CXX_host    = clang++
CC_host     = clang
CPP_host    = clang
LINK_host   = clang++
GYP_DEFINES = "clang=1"

SRC_FILES = $(wildcard src/*.cc)
TEST_EXCLUDES = tests/runner.cc, src/main.cc
TEST_FILES = $(filter-out $(TEST_EXCLUDES), $(wildcard tests/*.cc), $(SRC_FILES))


build:
	mkdir -p build
	$(CXX) -std=c++14 -stdlib=libc++ -o build/jsworker \
		-I /usr/local/include/mozjs-52/ \
		-L /usr/local/lib/ \
    	-I $(ROOT)/tests/include/ \
    	-I $(ROOT)/tests/include/ \
    	-I ${ROOT}/src \
		-lmozjs-52 -lz \
    	$(SRC_FILES)

test: test_compile tests/runner.o
	tests/runner

tests/runner.o: tests/runner.cc
	$(CXX) \
		-I $(ROOT)/tests \
		-I $(ROOT)/tests/include/ \
		-I $(V8) -I $(V8)include \
		-c -std=c++14 -stdlib=libc++ \
		tests/runner.cc \
		-o tests/runner.o

test_compile: tests/runner.o
	$(CXX) -I $(ROOT) -std=c++14 -stdlib=libc++ -o tests/runner \
	-I $(ROOT)/tests/include/ \
	-I $(ROOT)/tests/include/ \
	-I ${ROOT}/src \
	${TEST_FILES} \
	tests/runner.o \
	

format:
	clang-format -style=file -i $(SRC_FILES)


.PHONY: format test build
