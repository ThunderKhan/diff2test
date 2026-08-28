cmake_minimum_required(VERSION 3.20)
project(diff2test VERSION 0.1.0 LANGUAGES CXX)

add_executable(diff2test diff2test.cpp)
target_compile_features(diff2test PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(diff2test PRIVATE /W4 /permissive-)
else()
    target_compile_options(diff2test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()
