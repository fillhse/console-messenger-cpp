#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

constexpr int PORT = 9090;
constexpr int BUFFER_SIZE = 1024;

int accept_client(int server_fd) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        perror("Ошибка при принятии подключения");
        exit(EXIT_FAILURE);
    }
    return client_socket;
}

std::string recv_message(int socket_fd) {
    char buffer[BUFFER_SIZE]{};
    ssize_t bytes_read = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) return "";
    return std::string(buffer, bytes_read);
}

void send_message(int socket_fd, const std::string& message) {
    send(socket_fd, message.c_str(), message.size(), 0);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Ошибка при создании сокета");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка при привязке сокета");
        return 1;
    }

    if (listen(server_fd, 2) < 0) {
        perror("Ошибка при прослушивании");
        return 1;
    }

    std::cout << "[СЕРВЕР] Ожидание подключений...\n";

    int client1 = accept_client(server_fd);
    send_message(client1, "Введите свой Telegram ID:\n");
    std::string id1 = recv_message(client1);
    std::cout << "[СЕРВЕР] Подключён клиент 1: " << id1 << "\n";

    int client2 = accept_client(server_fd);
    send_message(client2, "Введите свой Telegram ID:\n");
    std::string id2 = recv_message(client2);
    std::cout << "[СЕРВЕР] Подключён клиент 2: " << id2 << "\n";

    send_message(client1, "Вы начинаете общение. Пишите сообщение. Для передачи хода введите /over\n");
    send_message(client2, "Ожидайте, пока собеседник закончит.\n");

    int sender = client1;
    int receiver = client2;

    while (true) {
        std::string msg = recv_message(sender);
        if (msg.empty()) {
            std::cout << "[СЕРВЕР] Один из клиентов отключился.\n";
            break;
        }

        if (msg == "/over\n" || msg == "/over") {
            send_message(sender, "Вы передали ход.\n");
            send_message(receiver, "Теперь ваша очередь. Для передачи хода введите /over\n");
            std::swap(sender, receiver);
        } else {
            std::string full_msg = "[СООБЩЕНИЕ] " + msg;
            send_message(receiver, full_msg);
        }
    }

    close(client1);
    close(client2);
    close(server_fd);
    return 0;
}
