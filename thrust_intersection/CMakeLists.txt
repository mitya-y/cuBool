cmake_minimum_required(VERSION 3.10)
project(test_thrust)
enable_language(CUDA)

set(SOURCES main.cpp test.cu)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})