#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <ctime>
#include <sstream>

constexpr int PORT = 9090;

struct ClientInfo {
    int fd;
    std::string id;
    std::string connected_to;
    bool is_speaking = false;
    std::string pending_request_from;
};

std::unordered_map<int, ClientInfo> clients;
std::unordered_map<std::string, int> id_to_fd;

std::string get_timestamp() {
    time_t now = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&now));
    return std::string(buf);
}

void disconnect_client(int fd, fd_set &master_fds) {
    if (clients.count(fd)) {
        std::string id = clients[fd].id;
        std::string connected_to = clients[fd].connected_to;
        std::cout << "Disconnecting client: " << id << " (fd: " << fd << ")\n";

        if (!connected_to.empty() && id_to_fd.count(connected_to)) {
            int target_fd = id_to_fd[connected_to];
            clients[target_fd].connected_to.clear();
            clients[target_fd].is_speaking = false;
            std::string msg = "Your conversation partner has left the chat.\n";
            send(target_fd, msg.c_str(), msg.size(), 0);
        }

        clients.erase(fd);
        id_to_fd.erase(id);
        FD_CLR(fd, &master_fds);
        close(fd);
    }
}

void handle_client_command(int fd, const std::string &msg, fd_set &master_fds) {
    if (msg.rfind("/connect ", 0) == 0) {
        std::string target_id = msg.substr(9);
        if (id_to_fd.count(target_id)) {
            int target_fd = id_to_fd[target_id];

            if (!clients[target_fd].pending_request_from.empty()) {
                send(fd, "User is busy with another request.\n", 34, 0);
                return;
            }

            if (!clients[target_fd].connected_to.empty()) {
                std::string notice = "User '" + clients[fd].id + "' attempted to connect to you, but you are already in a conversation.\n";
                send(target_fd, notice.c_str(), notice.size(), 0);

                send(fd, "User is already connected.\n", 29, 0);
                return;
            }

            clients[target_fd].pending_request_from = clients[fd].id;
            std::string prompt = "User '" + clients[fd].id + "' wants to connect. Accept? (yes/no)\n";
            send(target_fd, prompt.c_str(), prompt.size(), 0);
        } else {
            send(fd, "User not found.\n", 17, 0);
        }
    } else if (msg == "/vote") {
        if (clients[fd].is_speaking) {
            std::string target_id = clients[fd].connected_to;
            if (!target_id.empty() && id_to_fd.count(target_id)) {
                int target_fd = id_to_fd[target_id];
                clients[fd].is_speaking = false;
                clients[target_fd].is_speaking = true;
                send(fd, "You passed the microphone.\n", 28, 0);
                send(target_fd, "You are now speaking.\n", 24, 0);
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

void handle_pending_response(int fd, const std::string &msg) {
    ClientInfo &responder = clients[fd];
    if (responder.pending_request_from.empty()) return;

    std::string requester_id = responder.pending_request_from;
    responder.pending_request_from.clear();

    if (!id_to_fd.count(requester_id)) {
        send(fd, "Requester disconnected.\n", 25, 0);
        return;
    }

    int requester_fd = id_to_fd[requester_id];
    if (msg == "yes") {
        responder.connected_to = requester_id;
        clients[requester_fd].connected_to = responder.id;
        clients[requester_fd].is_speaking = true;
        send(requester_fd, "Connection accepted. You are now speaking.\n", 45, 0);
        send(fd, "Connection established.\n", 26, 0);
    } else {
        send(requester_fd, "Connection rejected.\n", 23, 0);
        send(fd, "Connection declined.\n", 23, 0);
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
                        } else if (!clients[fd].pending_request_from.empty()) {
                            handle_pending_response(fd, msg);
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
                                std::string timestamp = get_timestamp();
                                std::string sender = clients[fd].id;
                                std::string text = "[" + timestamp + "] " + sender + ": " + msg + "\n";
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
