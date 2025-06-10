cmake_minimum_required(VERSION 3.10)
project(console_messenger)

set(CMAKE_CXX_STANDARD 17)

# Сервер
add_executable(server
    server/main_server.cpp
    server/history.cpp
    server/telegram_auth.cpp
)

# Клиент
add_executable(client
    client/main_client.cpp
)

#Для std::filesystem (если GCC 8–9)
target_link_libraries(server stdc++fs)
