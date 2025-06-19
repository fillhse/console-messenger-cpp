cmake_minimum_required(VERSION 3.16)
project(console_messenger LANGUAGES CXX)

# ── C++ standard ───────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD      20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS    OFF)
add_compile_options(-Wall -Wextra -Wpedantic)

# ── External dependencies ───────────────────────────────────────────────────────
find_package(CURL    REQUIRED)       # system libcurl
find_package(Threads REQUIRED)       # portable threading

# Force cpr to use system curl/libcurl rather than building its own
set(CPR_USE_SYSTEM_CURL    ON CACHE BOOL "" FORCE)
set(CPR_USE_SYSTEM_LIBCURL ON CACHE BOOL "" FORCE)

# cpr: place the repo under extern/cpr, then:
#   git submodule add https://github.com/libcpr/cpr.git extern/cpr
#   git submodule update --init --recursive
add_subdirectory(extern/cpr)         # provides target cpr::cpr

# ── Core library ───────────────────────────────────────────────────────────────
add_library(project_libs STATIC
    server/history.cpp
    server/telegram_auth.cpp
)
target_link_libraries(project_libs
    PUBLIC
        CURL::libcurl
        cpr::cpr
        Threads::Threads
)
target_include_directories(project_libs
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/server
        ${PROJECT_SOURCE_DIR}/client
)


# ── Executables ────────────────────────────────────────────────────────────────
add_executable(console_server
    server/main_server.cpp
)
target_link_libraries(console_server PRIVATE project_libs)

add_executable(console_client
    client/main_client.cpp
)
target_link_libraries(console_client PRIVATE project_libs)

# ── doctest (unit testing) ─────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
set(DOCTEST_WITH_MAIN OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(doctest)

enable_testing()

add_executable(run_tests
    tests/test_history.cpp
    tests/test_telegram_auth.cpp
    tests/test_main_client.cpp
    tests/test_main_server.cpp
)
target_link_libraries(run_tests
    PRIVATE
        project_libs
        doctest::doctest
        Threads::Threads
)
add_test(NAME unit_tests COMMAND run_tests)
