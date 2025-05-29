#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <thread>

constexpr int PORT = 9090;

void receive_messages(int sock_fd) {
    char buffer[1024];
    while (true) {
        int bytes = read(sock_fd, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) {
            std::cout << "\nDisconnected from server.\n";
            break;
        }
        buffer[bytes] = '\0';
        std::string msg(buffer);
        std::cout << msg;
        std::cout.flush();
    }
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection to server failed");
        return 1;
    }

    std::thread recv_thread(receive_messages, sock_fd);

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "/exit") {
            break;
        }
        input += "\n";
        send(sock_fd, input.c_str(), input.size(), 0);
    }

    close(sock_fd);
    recv_thread.join();
    return 0;
}