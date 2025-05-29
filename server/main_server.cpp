#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <cstring>
#include <map>
#include <unordered_map>
#include <deque>
#include <vector>
#include <algorithm>

constexpr int PORT = 9090;

struct ClientInfo {
    int fd;
    std::string id;
    std::string connected_to;
    bool is_speaking = false;
};

std::unordered_map<int, ClientInfo> clients;
std::unordered_map<std::string, int> id_to_fd;
std::deque<int> speaking_queue;

void promote_next_speaker() {
    while (!speaking_queue.empty()) {
        int next_fd = speaking_queue.front();
        speaking_queue.pop_front();
        if (clients.count(next_fd)) {
            clients[next_fd].is_speaking = true;
            std::string msg = "You are now speaking.\n";
            send(next_fd, msg.c_str(), msg.size(), 0);
            return;
        }
    }
}

void disconnect_client(int fd, fd_set &master_fds) {
    if (clients.count(fd)) {
        std::string id = clients[fd].id;
        std::string connected_to = clients[fd].connected_to;
        std::cout << "Disconnecting client: " << id << " (fd: " << fd << ")\n";

        if (!connected_to.empty() && id_to_fd.count(connected_to)) {
            int target_fd = id_to_fd[connected_to];
            clients[target_fd].connected_to.clear();
            std::string msg = "Your conversation partner has left the chat.\n";
            send(target_fd, msg.c_str(), msg.size(), 0);
        }

        if (clients[fd].is_speaking) {
            clients[fd].is_speaking = false;
            promote_next_speaker();
        }

        clients.erase(fd);
        id_to_fd.erase(id);
        FD_CLR(fd, &master_fds);
        close(fd);
    }
}

void try_add_to_queue(int fd) {
    if (!clients[fd].is_speaking && std::find(speaking_queue.begin(), speaking_queue.end(), fd) == speaking_queue.end()) {
        speaking_queue.push_back(fd);
        std::string notify = "You have been added to the speaking queue.\n";
        send(fd, notify.c_str(), notify.size(), 0);

        bool someone_speaking = false;
        for (const auto& [_, client] : clients) {
            if (client.is_speaking) someone_speaking = true;
        }
        if (!someone_speaking) {
            promote_next_speaker();
        }
    }
}

void handle_client_command(int fd, const std::string &msg, fd_set &master_fds) {
    if (msg.rfind("/connect ", 0) == 0) {
        std::string target_id = msg.substr(9);
        if (id_to_fd.count(target_id)) {
            int target_fd = id_to_fd[target_id];
            if (!clients[target_fd].connected_to.empty()) {
                int old_fd = id_to_fd[clients[target_fd].connected_to];
                clients[old_fd].connected_to.clear();
                std::string warn = "Your session was terminated by another connection.\n";
                send(old_fd, warn.c_str(), warn.size(), 0);
            }
            clients[fd].connected_to = target_id;
            clients[target_fd].connected_to = clients[fd].id;
            send(fd, "Connection established.\n", 26, 0);
            send(target_fd, "A user connected to you.\n", 26, 0);
            try_add_to_queue(fd);
        } else {
            send(fd, "User not found.\n", 17, 0);
        }
    } else if (msg == "/vote") {
        if (clients[fd].is_speaking) {
            std::string target_id = clients[fd].connected_to;
            if (!target_id.empty() && id_to_fd.count(target_id)) {
                int target_fd = id_to_fd[target_id];
                speaking_queue.push_front(target_fd);
                clients[fd].is_speaking = false;
                send(fd, "You passed the microphone.\n", 28, 0);
                promote_next_speaker();
            } else {
                send(fd, "No connected client to pass speaking right.\n", 44, 0);
            }
        } else {
            send(fd, "You are not the current speaker.\n", 34, 0);
        }
    } else if (msg == "/end") {
        disconnect_client(fd, master_fds);
    } else {
        send(fd, "Only /connect <ID>, /vote, /end are allowed.\n", 45, 0);
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(listener, SOMAXCONN);
    std::cout << "Server listening on port " << PORT << std::endl;

    fd_set master_fds, read_fds;
    FD_ZERO(&master_fds);
    FD_SET(listener, &master_fds);
    int fd_max = listener;

    while (true) {
        read_fds = master_fds;
        if (select(fd_max + 1, &read_fds, nullptr, nullptr, nullptr) == -1) {
            perror("select");
            break;
        }

        for (int fd = 0; fd <= fd_max; ++fd) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == listener) {
                    sockaddr_in client_addr{};
                    socklen_t addrlen = sizeof(client_addr);
                    int client_fd = accept(listener, (sockaddr*)&client_addr, &addrlen);
                    if (client_fd != -1) {
                        FD_SET(client_fd, &master_fds);
                        fd_max = std::max(fd_max, client_fd);
                        send(client_fd, "Enter your ID: ", 16, 0);
                    }
                } else {
                    char buf[1024];
                    int bytes_read = read(fd, buf, sizeof(buf) - 1);
                    if (bytes_read <= 0) {
                        disconnect_client(fd, master_fds);
                    } else {
                        buf[bytes_read] = '\0';
                        std::string msg(buf);
                        msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
                        msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());

                        if (clients.count(fd) == 0) {
                            std::string id = msg;
                            if (id_to_fd.count(id)) {
                                int old_fd = id_to_fd[id];
                                send(old_fd, "Your ID is used by another client.\n", 37, 0);
                                disconnect_client(old_fd, master_fds);
                            }
                            clients[fd] = ClientInfo{fd, id};
                            id_to_fd[id] = fd;
                            std::string welcome = "Hello, " + id + "! Use /connect <ID>, /vote, /end\n";
                            send(fd, welcome.c_str(), welcome.size(), 0);
                        } else if (msg[0] == '/') {
                            handle_client_command(fd, msg, master_fds);
                        } else {
                            if (!clients[fd].is_speaking) {
                                send(fd, "You cannot send messages unless you're the current speaker.\n", 63, 0);
                                continue;
                            }
                            std::string target_id = clients[fd].connected_to;
                            if (!target_id.empty() && id_to_fd.count(target_id)) {
                                int target_fd = id_to_fd[target_id];
                                std::string text = msg + "\n";
                                send(target_fd, text.c_str(), text.size(), 0);
                            } else {
                                send(fd, "Not connected. Use /connect <ID>\n", 33, 0);
                            }
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}
