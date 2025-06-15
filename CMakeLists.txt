cmake_minimum_required(VERSION 3.10)
project(console_messenger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- общий include-путь: корень проекта --------------------
include_directories("${PROJECT_SOURCE_DIR}")

# ---------------- Сервер ----------------------------------
add_executable(server
    server/main_server.cpp
    server/history.cpp
    server/telegram_auth.cpp
)
# GCC 8-9: <filesystem>
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    target_link_libraries(server PRIVATE stdc++fs)
endif()
if (UNIX AND NOT APPLE)
    target_link_libraries(server PRIVATE pthread)
endif()

# ---------------- Клиент ----------------------------------
add_executable(client
    client/main_client.cpp
)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
    target_link_libraries(client PRIVATE stdc++fs)
endif()
if (UNIX AND NOT APPLE)
    target_link_libraries(client PRIVATE pthread)
endif()
