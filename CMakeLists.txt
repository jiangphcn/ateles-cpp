cmake_minimum_required (VERSION 2.6)
project (ateles)

# Configure dependencies
# Eventually we'll want to expand the mozjs version range
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOZJS REQUIRED mozjs-52)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
pkg_check_modules(GRPC REQUIRED grpc++)

find_program(PROTOC protoc)
find_program(GRPC_PLUGIN grpc_cpp_plugin)

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(ATELES_PB_SRC ${PROTO_DIR}/ateles.proto)
set(ATELES_PB ${CMAKE_CURRENT_BINARY_DIR}/ateles.pb.cc)
set(ATELES_GRPC ${CMAKE_CURRENT_BINARY_DIR}/ateles.grpc.pb.cc)

add_custom_command (
    OUTPUT ${ATELES_PB}
    COMMAND ${PROTOC}
        -I ${PROTO_DIR}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        ateles.proto
    DEPENDS ${ATELES_PB_SRC}
)

add_custom_command(
    OUTPUT ${ATELES_GRPC}
    COMMAND ${PROTOC}
        -I ${PROTO_DIR}
        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=${GRPC_PLUGIN}
        ateles.proto
    DEPENDS ${ATELES_PB_SRC}
)


file(GLOB SRC "src/*.cc")
add_executable(ateles ${ATELES_PB} ${ATELES_GRPC} ${SRC})

set(CFLAGS
    -std=c++14
    -Wall
    -Werror
    ${MOZJS_CFLAGS}
    ${PROTOBUF_CFLAGS}
    ${GRPC_CFLAGS})

set(INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}
    ${MOZJS_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS})

set(LDFLAGS
    ${MOZJS_LDFLAGS}
    ${PROTOBUF_LDFLAGS}
    ${GRPC_LDFLAGS}
    -lgrpc++_reflection)

target_compile_options(ateles PUBLIC ${CFLAGS})
target_include_directories(ateles PUBLIC ${INCLUDES})
target_link_libraries(ateles ${LDFLAGS})
