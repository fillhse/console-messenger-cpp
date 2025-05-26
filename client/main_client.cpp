#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 9090;
constexpr int BUFFER_SIZE = 1024;

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Ошибка при создании сокета");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка подключения к серверу");
        return 1;
    }

    char buffer[BUFFER_SIZE]{};
    fd_set readfds;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sock_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_fd = std::max(sock_fd, STDIN_FILENO) + 1;
        if (select(max_fd, &readfds, nullptr, nullptr, nullptr) < 0) {
            perror("Ошибка select()");
            break;
        }

        if (FD_ISSET(sock_fd, &readfds)) {
            ssize_t bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                std::cout << "Сервер отключился\n";
                break;
            }
            buffer[bytes] = '\0';
            std::cout << buffer << std::flush;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string input;
            std::getline(std::cin, input);
            input += '\n';
            send(sock_fd, input.c_str(), input.size(), 0);
        }
    }

    close(sock_fd);
    return 0;
}
