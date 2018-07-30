cmake_minimum_required(VERSION 3.5)
project(shared_memory)

option(SHARED_MEMORY_TESTS "Run unit tests" OFF)
if(SHARED_MEMORY_TESTS)
  add_subdirectory(test)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

