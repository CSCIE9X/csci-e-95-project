cmake_minimum_required(VERSION 3.6)
project(e95_course)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/compiler.c
    src/compiler.h
    src/ir.c
    src/ir.h
    src/mips.c
    src/mips.h
    src/node.c
    src/node.h
    src/parser.h
    src/scanner.h
    src/symbol.c
    src/symbol.h
    src/type.c
    src/type.h)

add_executable(e95_course ${SOURCE_FILES})