cmake_minimum_required(VERSION 3.20)
project(slipstream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(slipstream INTERFACE)
target_include_directories(slipstream INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(slipstream INTERFACE -Wall -Wextra -Wpedantic)

add_executable(slipstream_app 
    src/main.cpp
    src/cli/cli_config.cpp
)
target_link_libraries(slipstream_app PRIVATE slipstream)

option(SLIPSTREAM_BUILD_TESTS "build unit tests" ON)
if(SLIPSTREAM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()