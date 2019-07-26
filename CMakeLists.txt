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
find_program(PYTHON python)

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


# Generate our JS data files
set(MKHEADER ${CMAKE_CURRENT_SOURCE_DIR}/tools/mkheader.py)
set(ESCODEGEN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/js/escodegen.js)
set(ESCODEGEN_HDR ${CMAKE_CURRENT_BINARY_DIR}/escodegen.h)
set(ESPRIMA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/js/esprima.js)
set(ESPRIMA_HDR ${CMAKE_CURRENT_BINARY_DIR}/esprima.h)
set(REWRITE_ANON_FUN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/js/rewrite_anon_fun.js)
set(REWRITE_ANON_FUN_HDR ${CMAKE_CURRENT_BINARY_DIR}/rewrite_anon_fun.h)
set(ATELES_MAP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/js/ateles_map.js)
set(ATELES_MAP_HDR ${CMAKE_CURRENT_BINARY_DIR}/ateles_map.h)


add_custom_command(
    OUTPUT ${ESCODEGEN_HDR}
    COMMAND ${PYTHON} ${MKHEADER} ${ESCODEGEN_SRC} ${ESCODEGEN_HDR} escodegen
    DEPENDS ${ESCODEGEN_SRC}
)

add_custom_command(
    OUTPUT ${ESPRIMA_HDR}
    COMMAND ${PYTHON} ${MKHEADER} ${ESPRIMA_SRC} ${ESPRIMA_HDR} esprima
    DEPENDS ${ESPRIMA_SRC}
)

add_custom_command(
    OUTPUT ${REWRITE_ANON_FUN_HDR}
    COMMAND ${PYTHON}
        ${MKHEADER}
        ${REWRITE_ANON_FUN_SRC}
        ${REWRITE_ANON_FUN_HDR}
        rewrite_anon_fun
        DEPENDS ${REWRITE_ANON_FUN_SRC}
)

add_custom_command(
    OUTPUT ${ATELES_MAP_HDR}
    COMMAND ${PYTHON} ${MKHEADER} ${ATELES_MAP_SRC} ${ATELES_MAP_HDR} ateles_map
    DEPENDS ${ATELES_MAP_SRC}
)

set(
    JS_HDR_LIST
    ${ESCODEGEN_HDR}
    ${ESPRIMA_HDR}
    ${REWRITE_ANON_FUN_HDR}
    ${ATELES_MAP_HDR}
)


file(GLOB SRC "src/*.cc")
add_executable(ateles ${ATELES_PB} ${ATELES_GRPC} ${JS_HDR_LIST} ${SRC})

set(CFLAGS
    -g
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
