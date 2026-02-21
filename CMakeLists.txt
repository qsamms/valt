cmake_minimum_required(VERSION 3.10)
project(Valt CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(CLI11 REQUIRED)

pkg_check_modules(JEMALLOC REQUIRED jemalloc)

find_path(LIBEV_INCLUDE_DIR ev.h)
find_library(LIBEV_LIBRARY ev)

if(NOT LIBEV_INCLUDE_DIR OR NOT LIBEV_LIBRARY)
    message(FATAL_ERROR "libev not found on this system")
endif()

add_executable(
    valt
    src/main.cpp 
    src/net/network_layer.cpp
    src/valt/valt.cpp
    src/db/database_mixin.cpp
    src/pubsub/pubsub_mixin.cpp
    src/perms/permissions_mixin.cpp
    src/utils/utils.cpp
)

target_include_directories(valt PRIVATE src/ ${LIBEV_INCLUDE_DIR} ${JEMALLOC_INCLUDE_DIRS})
target_link_libraries(valt PRIVATE 
    ${LIBEV_LIBRARY}
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
    ${JEMALLOC_LIBRARIES}
)
