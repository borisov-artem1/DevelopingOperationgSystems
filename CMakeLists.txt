cmake_minimum_required(VERSION 3.30)
project(DevelopingOperatingSystems)

set(CMAKE_CXX_STANDARD 20)

add_executable(DevelopingOperatingSystems lab1/main.cpp
        lab1/peterson_algorithm.h
        lab1/lamport_algorithm.h
)
