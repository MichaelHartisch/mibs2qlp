cmake_minimum_required(VERSION 3.14)

project(mibs2qlp LANGUAGES CXX)

add_executable(mibs2qlp mibs2qlp.cpp)
target_compile_features(mibs2qlp PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(mibs2qlp PRIVATE /W4)
else()
    target_compile_options(mibs2qlp PRIVATE -Wall -Wextra -pedantic)
endif()
