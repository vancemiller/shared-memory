cmake_minimum_required(VERSION 3.5)
project(shared-memory)

option(SHARED_MEMORY_TESTS "Run unit tests" OFF)
if(SHARED_MEMORY_TESTS)
  add_subdirectory(test)
endif()

if(NOT TARGET file-descriptor)
  add_subdirectory(file-descriptor)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)
target_link_libraries(${PROJECT_NAME} INTERFACE rt file-descriptor)
