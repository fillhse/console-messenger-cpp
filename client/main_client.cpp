#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <filesystem>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "socket_utils.h"

constexpr size_t MAX_INPUT = 2000;
const std::string CFG_DIR  = "CLIENT_SETTING";
const std::string CFG_FILE = "CLIENT_SETTING/ip_port.txt";

struct ServerConf { std::string ip; int port; };

bool valid_ip_port(const std::string& ip, int port) {
    sockaddr_in tmp{};
    return inet_pton(AF_INET, ip.c_str(), &tmp.sin_addr) == 1 && port > 0 && port < 65536;
}

ServerConf get_config() {
    std::filesystem::create_directories(CFG_DIR);
    std::ifstream fin(CFG_FILE);
    std::string ip; int port;
    bool ok = false;
    if (fin) {
        std::string line; std::getline(fin, line);
        std::istringstream ss(line);
        std::getline(ss, ip, ':'); ss >> port;
        ok = valid_ip_port(ip, port);
    }
    while (!ok) {
        std::cout << "Enter server IP: "; std::cin >> ip;
        std::cout << "Enter server port: "; std::cin >> port; std::cin.ignore();
        ok = valid_ip_port(ip, port);
        if (!ok) std::cout << "Invalid IP or port. Try again.\n";
    }
    std::ofstream(CFG_FILE, std::ios::trunc) << ip << ':' << port << '\n';
    return {ip, port};
}

void receive_messages(int fd) {
    std::string line;
    while (recv_line(fd, line)) std::cout << line << '\n';
    std::cout << "\nDisconnected from server.\n";
    close(fd);
    _exit(0);
}

int main() {
    ServerConf conf = get_config();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(conf.port);
    inet_pton(AF_INET, conf.ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); return 1; }

    std::thread(receive_messages, sock).detach();

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input.empty()) continue;
        if (input.size() > MAX_INPUT) {
            std::cout << "Message longer than 2000 characters. Split it.\n";
            continue;
        }
        if (input == "/exit") {
            send_line(sock, "/exit");
            std::cout << "\nExiting...\n";
            break;
        }
        send_line(sock, input);
    }
    close(sock);
    return 0;
}
