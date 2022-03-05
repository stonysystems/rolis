cmake_minimum_required(VERSION 3.6)
project(silo_ons)

set(CMAKE_CXX_STANDARD 11)

include_directories(
    masstree
)

file(GLOB SOURCE_FILES
    "benchmarks/sto/*.cc"
    "benchmarks/sto/*.hh"
    benchmarks/sto/*.c
        benchmarks/sto/*.cpp
    benchmarks/sto/*.h
    benchmarks/*.h
    benchmarks/*.cc
    benchmarks/*.hh
    config/*.h
    masstree/*.hh
    masstree/*.cc
    masstree/*.c
    masstree/*.h
    record/*.h
    third-party/lz4/*.c
    third-party/lz4/*.h
    *.cc
    *.h
    *.hh)

add_executable(silo_ons ${SOURCE_FILES})