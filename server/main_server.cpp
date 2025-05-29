#include <iostream>
#include <algorithm> // ← сюда подключается std::remove
#include <string>
#include <cstring>
#include <unordered_map>
#include <set>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

constexpr int PORT = 9090;
constexpr int BUFFER_SIZE = 1024;

std::unordered_map<std::string, int> id_to_socket;
std::unordered_map<int, std::string> socket_to_id;
std::set<int> busy;

std::string recv_message(int socket_fd) {
    char buffer[BUFFER_SIZE]{};
    ssize_t bytes_read = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) return "";
    return std::string(buffer, bytes_read);
}

void send_message(int socket_fd, const std::string& msg) {
    send(socket_fd, msg.c_str(), msg.size(), 0);
}

void handle_chat(int sock1, int sock2, const std::string& id1, const std::string& id2) {
    send_message(sock1, "Вы начинаете чат с " + id2 + ". Для передачи хода введите /over\n");
    send_message(sock2, "Ожидайте хода от " + id1 + ".\n");

    int sender = sock1;
    int receiver = sock2;
    while (true) {
        std::string msg = recv_message(sender);
        if (msg.empty()) break;

        if (msg == "/over\n" || msg == "/over") {
            send_message(sender, "Вы передали ход.\n");
            send_message(receiver, "Теперь ваша очередь. Введите /over, чтобы передать ход обратно.\n");
            std::swap(sender, receiver);
        } else {
            std::string full = "[MSG] " + msg;
            send_message(receiver, full);
        }
    }

    send_message(sock1, "Чат завершён.\n");
    send_message(sock2, "Чат завершён.\n");
    busy.erase(sock1);
    busy.erase(sock2);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Сокет не создан");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);
    std::cout << "[СЕРВЕР] Готов к подключениям на порту " << PORT << "\n";

    fd_set read_fds;
    int max_fd = server_fd;

    std::vector<int> sockets;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        for (int sock : sockets) FD_SET(sock, &read_fds);

        select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd > 0) {
                send_message(client_fd, "Введите ваш Telegram ID:\n");
                std::string id = recv_message(client_fd);
                id.erase(id.find_last_not_of("\r\n") + 1);

                id_to_socket[id] = client_fd;
                socket_to_id[client_fd] = id;
                sockets.push_back(client_fd);
                std::cout << "[+] Подключён ID: " << id << "\n";
                send_message(client_fd, "Добро пожаловать. Используйте /connect <ID>, чтобы начать чат.\n");
                if (client_fd > max_fd) max_fd = client_fd;
            }
        }

        for (int sock : sockets) {
            if (FD_ISSET(sock, &read_fds)) {
                std::string msg = recv_message(sock);
                if (msg.empty()) {
                    std::cout << "[-] Отключился: " << socket_to_id[sock] << "\n";
                    close(sock);
                    sockets.erase(std::remove(sockets.begin(), sockets.end(), sock), sockets.end());
                    id_to_socket.erase(socket_to_id[sock]);
                    socket_to_id.erase(sock);
                    busy.erase(sock);
                    continue;
                }

                msg.erase(msg.find_last_not_of("\r\n") + 1);

                if (msg.rfind("/connect ", 0) == 0) {
                    std::string target_id = msg.substr(9);
                    if (id_to_socket.count(target_id) == 0) {
                        send_message(sock, "Пользователь не найден.\n");
                        continue;
                    }
                    int target_fd = id_to_socket[target_id];
                    if (busy.count(target_fd)) {
                        send_message(sock, "Пользователь занят.\n");
                        continue;
                    }
                    std::string from_id = socket_to_id[sock];
                    send_message(target_fd, "ID " + from_id + " хочет с вами связаться. Принять? (yes/no)\n");
                    send_message(sock, "Ожидаем ответ собеседника...\n");

                    std::string response = recv_message(target_fd);
                    response.erase(response.find_last_not_of("\r\n") + 1);

                    if (response == "yes") {
                        send_message(sock, "Собеседник согласился. Начинаем чат...\n");
                        send_message(target_fd, "Чат подтверждён. Начинаем...\n");
                        busy.insert(sock);
                        busy.insert(target_fd);
                        handle_chat(sock, target_fd, from_id, target_id);
                    } else {
                        send_message(sock, "Собеседник отказался от чата.\n");
                        send_message(target_fd, "Вы отказались от чата.\n");
                    }
                } else {
                    send_message(sock, "Ожидание команды. Для начала чата: /connect <ID>\n");
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
