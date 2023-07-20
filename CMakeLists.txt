cmake_minimum_required(VERSION 3.9)
# Keep the version below in sync with the one in db.h
project(MyDataBase VERSION 1.23.0 LANGUAGES C CXX)
include_directories(include)
include_directories(src)
set(CMAKE_C_STANDARD 20)
add_subdirectory(src)
