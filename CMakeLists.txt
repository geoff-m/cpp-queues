cmake_minimum_required(VERSION 3.22)
project(queues CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(lib)

add_subdirectory(thirdparty/googletest)

add_subdirectory(test)