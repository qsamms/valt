cmake_minimum_required(VERSION 3.10)
project(Valt CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    valt
    src/cb/cb.cpp
    src/db/db.cpp
    src/main.cpp 
    src/utils/response_codes.cpp
    src/task_handler/task_handler.cpp
    src/utils/utils.cpp
)

target_include_directories(valt PRIVATE src/ ${LIBEV_INCLUDE_DIRS})
target_link_libraries(valt PRIVATE ev)
