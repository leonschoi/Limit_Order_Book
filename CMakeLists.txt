cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json for VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the build type to Debug if not already specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Add executable
add_executable(LimitOrderBook src/main.cpp)

target_include_directories(LimitOrderBook PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_sources(LimitOrderBook PRIVATE
    src/common/parse_args.cpp
    src/common/pin_thread.cpp
    src/driver/run_benchmark.cpp
)
