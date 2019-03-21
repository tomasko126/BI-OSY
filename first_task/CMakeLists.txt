cmake_minimum_required(VERSION 3.13)
project(first_task)

set(CMAKE_CXX_STANDARD 14)

link_directories("/Users/tomastaro/Documents/GitHub/BI-OSY/first_task/")

add_executable(first_task common.h first_task.cpp progtest_solver.h sample_tester.cpp sample_tester.h)

find_library(progtest_solver progtest_solver x86_64-darwin18/libprogtest_solver.a)

target_link_libraries(first_task pthread ${progtest_solver})