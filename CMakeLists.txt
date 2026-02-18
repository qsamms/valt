cmake_minimum_required(VERSION 3.10)
project(Valt CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(
    valt
    src/main.cpp 
    src/cb/callbacks.cpp
    src/valt/valt.cpp
    src/db/database_mixin.cpp
    src/pubsub/pubsub_mixin.cpp
    src/perms/permissions_mixin.cpp
    src/utils/utils.cpp
)

target_include_directories(valt PRIVATE src/)
target_link_libraries(valt PRIVATE ev spdlog::spdlog OpenSSL::SSL OpenSSL::Crypto)
