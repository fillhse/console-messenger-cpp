cmake_minimum_required(VERSION 3.16)
project(console_messenger CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS   OFF)

include_directories(${PROJECT_SOURCE_DIR})

# ── приложения ───────────────────────────────────────────────────────────────
add_executable(console_server
    server/main_server.cpp
    server/history.cpp
    server/telegram_auth.cpp)
find_package(CURL REQUIRED)
target_link_libraries(console_server PRIVATE CURL::libcurl pthread)

add_executable(console_client client/main_client.cpp)
target_link_libraries(console_client PRIVATE pthread)

# ── библиотека с логикой для тестов ──────────────────────────────────────────
add_library(project_libs STATIC
    server/history.cpp
    server/telegram_auth.cpp)
target_link_libraries(project_libs PUBLIC CURL::libcurl pthread)

# ── doctest ──────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11)
FetchContent_MakeAvailable(doctest)

# ── unit-tests ───────────────────────────────────────────────────────────────
enable_testing()

add_executable(run_tests
    tests/test_history.cpp          # <-- содержит main()
    tests/test_telegram_auth.cpp
    tests/test_main_client.cpp
    tests/test_main_server.cpp)
target_link_libraries(run_tests
    PRIVATE project_libs doctest::doctest pthread)

add_test(NAME unit_tests COMMAND run_tests)
