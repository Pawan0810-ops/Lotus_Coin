cmake_minimum_required(VERSION 3.14)
project(LotusCoin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(include ${OPENSSL_INCLUDE_DIR})

file(GLOB SOURCES "src/*.cpp")

add_executable(lotus_node ${SOURCES})

target_link_libraries(lotus_node PRIVATE 
    OpenSSL::Crypto 
    leveldb 
    Threads::Threads
)