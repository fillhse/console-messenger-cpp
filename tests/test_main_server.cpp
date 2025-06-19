#include <sys/select.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>

static std::map<int, std::string> g_sent;

inline int send_packet(int fd, const char* data) {
	g_sent[fd] += data;
	return 0;
}
inline int send_all(int fd, const char* data) {
	g_sent[fd] += data;
	return 0;
}

#define main main_server_entry
#include "../server/main_server.cpp"
#undef main

#include "doctest/doctest.h"

static void clear_state() {
	clients.clear();
	id_to_fd.clear();
	g_sent.clear();
}

TEST_SUITE("main_server::handle_client_command") {
	TEST_CASE("connect sets pending_request_from") {
		clear_state();
		fd_set master;
		FD_ZERO(&master);

		int fd1 = 1, fd2 = 2;
		clients[fd1] = {fd1, "123"};
		clients[fd2] = {fd2, "456"};
		id_to_fd["123"] = fd1;
		id_to_fd["456"] = fd2;

		handle_client_command(fd1, "/connect 456", master);
		REQUIRE(clients[fd2].pending_request_from == "123");
	}

	TEST_CASE("vote transfers speaking role") {
		clear_state();
		fd_set master;
		FD_ZERO(&master);

		int fd1 = 3, fd2 = 4;
		clients[fd1] = {fd1, "123", "456", true};
		clients[fd2] = {fd2, "456", "123", false};
		id_to_fd["123"] = fd1;
		id_to_fd["456"] = fd2;

		handle_client_command(fd1, "/vote", master);
		CHECK_FALSE(clients[fd1].is_speaking);
		CHECK(clients[fd2].is_speaking);
	}

	TEST_CASE("end clears connection for both sides") {
		clear_state();
		fd_set master;
		FD_ZERO(&master);

		int fd1 = 5, fd2 = 6;
		clients[fd1] = {fd1, "123", "456", true};
		clients[fd2] = {fd2, "456", "123", false};
		id_to_fd["123"] = fd1;
		id_to_fd["456"] = fd2;

		handle_client_command(fd1, "/end", master);
		CHECK(clients[fd1].connected_to.empty());
		CHECK(clients[fd2].connected_to.empty());
	}

	TEST_CASE("help sends help text") {
		clear_state();
		fd_set master;
		FD_ZERO(&master);

		int fd1 = 7;
		clients[fd1] = {fd1, "123"};
		id_to_fd["123"] = fd1;

		handle_client_command(fd1, "/help", master);
		CHECK(g_sent[fd1].find("Available commands") != std::string::npos);
	}

	TEST_CASE("unknown command replies error") {
		clear_state();
		fd_set master;
		FD_ZERO(&master);

		int fd1 = 8;
		clients[fd1] = {fd1, "123"};
		id_to_fd["123"] = fd1;

		handle_client_command(fd1, "/foo", master);
		CHECK(g_sent[fd1].find("Only /connect") != std::string::npos);
	}
}