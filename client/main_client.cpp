#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

constexpr int PORT = 9090;
constexpr int BUFFER_SIZE = 1024;

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Ошибка создания сокета");
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Не удалось подключиться к серверу");
        return 1;
    }

    fd_set read_fds;
    char buffer[BUFFER_SIZE]{};

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = std::max(sock_fd, STDIN_FILENO) + 1;
        if (select(max_fd, &read_fds, nullptr, nullptr, nullptr) < 0) break;

        if (FD_ISSET(sock_fd, &read_fds)) {
            ssize_t len = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
            if (len <= 0) break;
            buffer[len] = '\0';
            std::cout << buffer << std::flush;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::string input;
            std::getline(std::cin, input);
            input += "\n";
            send(sock_fd, input.c_str(), input.size(), 0);
        }
    }

    close(sock_fd);
    return 0;
}
