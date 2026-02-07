cmake_minimum_required(VERSION 3.10)
project(Valt CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)

add_executable(
    valt
    src/cb/callbacks.cpp
    src/main.cpp 
    src/task_handler/task_handler.cpp
    src/utils/utils.cpp
)

target_include_directories(valt PRIVATE src/ ${LIBEV_INCLUDE_DIRS})
target_link_libraries(valt PRIVATE ev spdlog::spdlog)
