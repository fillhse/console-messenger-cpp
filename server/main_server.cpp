#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "telegram_auth.h"

#include "history.h"
#include "socket_utils.h"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>

constexpr int PORT = 9090;

struct ClientInfo {
	int fd;
	std::string id;
	std::string connected_to;
	bool is_speaking = false;
	std::string pending_request_from;
};

static std::unordered_map<int, ClientInfo> clients;
static std::unordered_map<std::string, int> id_to_fd;
static std::unordered_map<int, std::string> pending_auth;

std::string get_timestamp() {
	time_t now = time(nullptr);
	char buf[20];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&now));
	return std::string(buf);
}

void disconnect_client(int fd, fd_set& master_fds) {
	if (clients.count(fd)) {
		std::string id = clients[fd].id;
		std::string connected_to = clients[fd].connected_to;
		std::cout << "\nDisconnecting client: " << id << " (fd: " << fd << ")\n";

		if (!connected_to.empty() && id_to_fd.count(connected_to)) {
			int target_fd = id_to_fd[connected_to];
			clients[target_fd].connected_to.clear();
			clients[target_fd].is_speaking = false;
			const std::string msg = "\nYour conversation partner has left the chat.\n";
			send_packet(target_fd, msg.c_str());
		}

		clients.erase(fd);
		id_to_fd.erase(id);
		FD_CLR(fd, &master_fds);
		close(fd);
	}
}

void handle_client_command(int fd, const std::string& msg, fd_set& master_fds) {
	if (msg.rfind("/connect ", 0) == 0) {
		std::string target_id = msg.substr(9);
		if (id_to_fd.count(target_id)) {
			int target_fd = id_to_fd[target_id];

			if (!clients[target_fd].pending_request_from.empty()) {
				send_packet(fd, "User is busy with another request.\n");
				return;
			}

			if (!clients[target_fd].connected_to.empty()) {
				const std::string notice = "\nUser '" + clients[fd].id +
				                           "' attempted to connect to you, but you are "
				                           "already in a conversation.\n";
				send_packet(target_fd, notice.c_str());
				send_packet(fd, "User is already connected.\n");
				return;
			}

			clients[target_fd].pending_request_from = clients[fd].id;
			const std::string prompt = "\nUser '" + clients[fd].id + "' wants to connect. Accept? (yes/no)\n";
			send_packet(target_fd, prompt.c_str());
		} else {
			send_packet(fd, "User not found.\n");
		}
	} else if (msg == "/vote") {
		if (clients[fd].is_speaking) {
			std::string target_id = clients[fd].connected_to;
			if (!target_id.empty() && id_to_fd.count(target_id)) {
				int target_fd = id_to_fd[target_id];
				clients[fd].is_speaking = false;
				clients[target_fd].is_speaking = true;
				send_all(fd, "You passed the microphone.\n");
				send_packet(target_fd, "You are now speaking.\n");
			} else {
				send_packet(fd, "No connected client to pass speaking right.\n");
			}
		} else {
			send_packet(fd, "You are not the current speaker.\n");
		}
	} else if (msg == "/end") {
		std::string partner_id = clients[fd].connected_to;
		if (!partner_id.empty() && id_to_fd.count(partner_id)) {
			int partner_fd = id_to_fd[partner_id];
			clients[partner_fd].connected_to.clear();
			clients[partner_fd].is_speaking = false;
			send_packet(partner_fd, "\nYour conversation partner has ended the chat.\n");
		}
		clients[fd].connected_to.clear();
		clients[fd].is_speaking = false;
		send_packet(fd, "You have left the conversation.\n");
	} else if (msg == "/help") {
		const std::string help =
		    "Available commands:\n"
		    "/connect <ID> - request chat with user\n"
		    "/vote         - pass speaker role\n"
		    "/end          - end current conversation\n"
		    "/exit         - exit the chat completely\n"
		    "/help         - show this message\n";
		send_packet(fd, help.c_str());
	} else if (msg == "/exit") {
		disconnect_client(fd, master_fds);
	} else {
		send_packet(fd, "Only /connect <ID>, /vote, /end, /exit, /help are allowed.\n");
	}
}

void handle_pending_response(int fd, const std::string& msg) {
	ClientInfo& responder = clients[fd];
	if (responder.pending_request_from.empty())
		return;

	std::string requester_id = responder.pending_request_from;
	responder.pending_request_from.clear();

	if (!id_to_fd.count(requester_id)) {
		send_packet(fd, "Requester disconnected.\n");
		return;
	}

	int requester_fd = id_to_fd[requester_id];
	if (msg == "yes") {
		std::cout << "Clients connected: " << responder.id << " <-> " << requester_id << std::endl;
		responder.connected_to = requester_id;
		clients[requester_fd].connected_to = responder.id;
		clients[requester_fd].is_speaking = true;

		std::string history = load_history_for_users(responder.id, requester_id);
		if (!history.empty()) {
			send_all(fd, "Chat history:\n");
			send_all(fd, history.c_str());
			send_all(requester_fd, "Chat history:\n");
			send_all(requester_fd, history.c_str());
		}
		send_packet(requester_fd, "Connection accepted. You are now speaking.\n");
		send_all(fd, "Connection established. You are a listener.\n");
	} else {
		send_packet(requester_fd, "Connection rejected.\n");
		send_packet(fd, "Connection declined.\n");
	}
}

int main() {
	ensure_bot_token();

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
	FD_SET(STDIN_FILENO, &master_fds);
	int fd_max = listener;

	while (true) {
		read_fds = master_fds;
		if (select(fd_max + 1, &read_fds, nullptr, nullptr, nullptr) == -1) {
			perror("select");
			break;
		}

		for (int fd = 0; fd <= fd_max; ++fd) {
			if (!FD_ISSET(fd, &read_fds))
				continue;

			if (fd == STDIN_FILENO) {
				std::string cmd;
				std::getline(std::cin, cmd);
				if (cmd == "/shutdown") {
					std::cout << "Shutting down server...\n";
					for (auto& [cfd, info] : clients)
						send_all(cfd, "\nServer is shutting down.\n");
					for (auto& [cfd, info] : clients)
						close(cfd);
					close(listener);
					std::cout << "Server stopped.\n";
					return 0;
				}
				continue;
			}

			if (fd == listener) {
				sockaddr_in client_addr{};
				socklen_t addrlen = sizeof(client_addr);
				int client_fd = accept(listener, (sockaddr*)&client_addr, &addrlen);
				if (client_fd != -1) {
					std::cout << "New client connected, fd: " << client_fd << std::endl;
					FD_SET(client_fd, &master_fds);
					fd_max = std::max(fd_max, client_fd);
					const char* ask_id = "Enter your ID\n";
					send_packet(client_fd, ask_id);
				}
			} else {
				std::string msg;
				if (!recv_line(fd, msg)) {
					disconnect_client(fd, master_fds);
					continue;
				}

				if (clients.count(fd) == 0 && !pending_auth.count(fd)) {
					std::string chat_id = msg;
					if (chat_id.empty()) {
						send_packet(fd, "Chat ID cannot be empty. Try again\n");
						continue;
					}

					std::string code = generate_auth_code();
					if (send_telegram_code(chat_id, code)) {
						pending_auth[fd] = chat_id;
						const char* sent = "Telegram code sent. Enter the code to log in\n";
						send_packet(fd, sent);
					} else {
						send_packet(fd,
						            "Failed to send Telegram message.\nUse command /exit to "
						            "exit.\nCheck the telegram ID and write it again");
					}
				}

				else if (pending_auth.count(fd)) {
					std::string entered_code = msg;
					std::string chat_id = pending_auth[fd];
					if (verify_auth_code(chat_id, entered_code)) {
						if (id_to_fd.count(chat_id)) {
							int old_fd = id_to_fd[chat_id];
							send_packet(old_fd, "\nYou have been logged out (second login detected).\n");
							disconnect_client(old_fd, master_fds);
						}

						clients[fd] = ClientInfo{fd, chat_id};
						std::cout << "Client authorized: " << chat_id << " (fd: " << fd << ")" << std::endl;
						id_to_fd[chat_id] = fd;
						pending_auth.erase(fd);

						std::string welcome =
						    "Welcome, " + chat_id + "! Use /connect <ID>, /vote, /end, /exit, /help\n";
						send_packet(fd, welcome.c_str());
					} else {
						send_packet(fd, "Incorrect code. Try again\n");
					}
				}

				else if (!clients[fd].pending_request_from.empty()) {
					handle_pending_response(fd, msg);
				}

				else if (!msg.empty() && msg[0] == '/') {
					handle_client_command(fd, msg, master_fds);
				}

				else {
					if (clients[fd].connected_to.empty()) {
						send_packet(fd,
						            "You are not in a conversation.\nUse /connect <ID> to "
						            "start chatting.\n");
						continue;
					}
					if (!clients[fd].is_speaking) {
						send_all(fd,
						         "You cannot send messages unless you're the current "
						         "speaker.\n");
						continue;
					}

					std::string target_id = clients[fd].connected_to;
					if (!target_id.empty() && id_to_fd.count(target_id)) {
						int target_fd = id_to_fd[target_id];
						std::string timestamp = get_timestamp();
						std::string sender = clients[fd].id;
						std::string text = "[" + timestamp + "] " + sender + ": " + msg + "\n";
						send_all(target_fd, text.c_str());
						append_message_to_history(sender, target_id, text);
					} else {
						send_packet(fd, "Not connected. Use /connect <ID>\n");
					}
				}
			}
		}
	}

	close(listener);
	return 0;
}
