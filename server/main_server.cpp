#include "history.h"
#include "telegram_auth.h"
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
std::unordered_map<int, std::string> pending_auth;

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
        std::string partner_id = clients[fd].connected_to;
        if (!partner_id.empty() && id_to_fd.count(partner_id)) {
            int partner_fd = id_to_fd[partner_id];
            clients[partner_fd].connected_to.clear();
            clients[partner_fd].is_speaking = false;
            send(partner_fd, "Your conversation partner has ended the chat.\n", 46, 0);
        }
        clients[fd].connected_to.clear();
        clients[fd].is_speaking = false;
        send(fd, "You have left the conversation.\n", 33, 0);
    } 
    else {
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

        std::string history = load_history_for_users(responder.id, requester_id);
        if (!history.empty()) {
            send(fd, "Chat history:\n", 14, 0);
            send(fd, history.c_str(), history.size(), 0);
            send(requester_fd, "Chat history:\n", 14, 0);
            send(requester_fd, history.c_str(), history.size(), 0);
        }
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
                        const char* ask_id = "Enter your ID: ";
                        send(client_fd, ask_id, strlen(ask_id), 0);
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

                        if (clients.count(fd) == 0 && !pending_auth.count(fd)) {
                            std::string chat_id = msg;
                            if (chat_id.empty()) {
                                send(fd, "Chat ID cannot be empty. Try again:\n", 36, 0);
                                continue;
                            }

                            std::string code = generate_auth_code();
                            if (send_telegram_code(chat_id, code)) {
                                pending_auth[fd] = chat_id;
                                const char* sent = "Telegram code sent. Enter the code to log in: ";
                                send(fd, sent, strlen(sent), 0);
                            } else {
                                send(fd, "Failed to send Telegram message.\nUse command /exit to exit.\nCheck the telegram ID and write it again: ", 104, 0);
                            }
                        }
                        else if (pending_auth.count(fd)) {
                            std::string entered_code = msg;
                            std::string chat_id = pending_auth[fd];
                            if (verify_auth_code(chat_id, entered_code)) {
                                clients[fd] = ClientInfo{fd, chat_id};
                                id_to_fd[chat_id] = fd;
                                pending_auth.erase(fd);

                                std::string welcome = "Welcome, " + chat_id + "! Use /connect <ID>, /vote, /end\n";
                                send(fd, welcome.c_str(), welcome.size(), 0);
                            } else {
                                send(fd, "Incorrect code. Try again: ", 27, 0);
                            }
                        } else if (!clients[fd].pending_request_from.empty()) {
                            handle_pending_response(fd, msg);
                        } else if (msg[0] == '/') {
                            handle_client_command(fd, msg, master_fds);
                        } else if (msg == "/help") {
                            std::string help =
                                "Available commands:\n"
                                "/connect <ID> - request chat with user\n"
                                "/vote         - pass speaker role\n"
                                "/end          - end current conversation\n"
                                "/exit         - exit the chat completely\n"
                                "/help         - show this message\n";
                            send(fd, help.c_str(), help.size(), 0);
                        }
                        else if (msg == "/exit") {
                            disconnect_client(fd, master_fds);
                        }
                        else {
                            if (clients[fd].connected_to.empty()) {
                                send(fd, "You are not in a conversation. Use /connect <ID> to start chatting.\n", 69, 0);
                                continue;
                            }
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
                                append_message_to_history(sender, target_id, text);
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
