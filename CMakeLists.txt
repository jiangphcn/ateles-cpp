cmake_minimum_required (VERSION 2.6)
project (ateles)

# Configure dependencies
# Eventually we'll want to expand the mozjs version range
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOZJS REQUIRED mozjs-52)


file(GLOB SRC "src/*.cc")
add_executable(ateles ${SRC})

set(CFLAGS -std=c++14 -Wall -Werror)
target_compile_options(ateles PUBLIC ${CFLAGS} ${MOZJS_CFLAGS_OTHER})
target_include_directories(ateles PUBLIC ${MOZJS_INCLUDE_DIRS})
target_link_libraries(ateles ${MOZJS_LDFLAGS})
