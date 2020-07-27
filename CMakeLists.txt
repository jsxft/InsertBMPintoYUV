cmake_minimum_required(VERSION 3.16)
project(InsertBMPintoYUV)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -msse4)

add_executable(InsertBMPintoYUV main.cpp image.cpp image.h utils.cpp utils.h)
