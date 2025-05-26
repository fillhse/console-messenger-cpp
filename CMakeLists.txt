cmake_minimum_required(VERSION 3.10)
project(RadioChat)

set(CMAKE_CXX_STANDARD 17)

add_executable(server server/main_server.cpp)
add_executable(client client/main_client.cpp)