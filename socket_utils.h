#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <string_view>

#include <string_view>

bool contains_forbidden_words(const std::string& str) {
	return str.find("*ENDM*") != std::string::npos;
}

inline bool send_all(int fd, std::string_view data) {
	std::string message(data);
	size_t sent = 0;
	while (sent < message.size()) {
		ssize_t n = ::send(fd, message.data() + sent, message.size() - sent, 0);
		if (n <= 0)
			return false;
		sent += static_cast<size_t>(n);
	}
	return true;
}

inline bool send_packet(int fd, std::string_view data)
{
	std::string message(data);	
    if (message.empty() || message.back() != '\n')
        message.push_back('\n');
	message += "*ENDM*\n"; 
    return send_all(fd, message);
}

inline bool send_line(int fd, std::string_view sv) {
	if (sv.empty() || sv.back() != '\n') {
		std::string tmp(sv);
		tmp.push_back('\n');
		return send_all(fd, tmp);
	}
	return send_all(fd, sv);
}

inline bool recv_line(int fd, std::string& out) {
	out.clear();
	char ch{};
	while (true) {
		ssize_t n = ::recv(fd, &ch, 1, 0);
		if (n <= 0)
			return false;
		if (ch == '\n')
			break;
		out.push_back(ch);
	}
	return true;
}

#endif
