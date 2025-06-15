#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <unistd.h>
#include <sys/socket.h>
#include <string_view>
#include <string>

inline bool send_all(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

inline bool send_all(int fd, std::string_view sv) {
    return send_all(fd, sv.data(), sv.size());
}

inline bool recv_line(int fd, std::string& out) {
    out.clear();
    char ch{};
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n <= 0) return false;
        if (ch == '\n') break;
        out.push_back(ch);
    }
    return true;
}

#endif
