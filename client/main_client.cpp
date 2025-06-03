#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>

constexpr int PORT = 9090;
constexpr const char* SERVER_IP = "127.0.0.1";

void receive_messages(int sockfd) {
    char buffer[1024];
    while (true) {
        int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cout << "\nDisconnected from server." << std::endl;
            close(sockfd);
            exit(0);
        }
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::flush;
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    std::thread receiver(receive_messages, sockfd);
    receiver.detach();

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input.empty()) continue;
        send(sockfd, input.c_str(), input.size(), 0);
    }

    close(sockfd);
    return 0;
}
