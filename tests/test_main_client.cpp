#include "doctest/doctest.h"
#define main main_client_entry
#include "../client/main_client.cpp"
#undef main

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

void reset_cfg_dir() {
	fs::remove_all("CLIENT_SETTING");
}

TEST_CASE("valid_ip_port basic") {
	CHECK(valid_ip_port("127.0.0.1", 80));
	CHECK_FALSE(valid_ip_port("256.0.0.1", 80));
	CHECK_FALSE(valid_ip_port("8.8.8.8", 70000));
}

TEST_CASE("get_config returns existing valid file") {
	reset_cfg_dir();
	fs::create_directories("CLIENT_SETTING");
	std::ofstream("CLIENT_SETTING/ip_port.txt") << "127.0.0.1:12345\n";

	ServerConf cfg = get_config();
	CHECK(cfg.ip == "127.0.0.1");
	CHECK(cfg.port == 12345);
}

TEST_CASE("get_config reparses invalid file and saves user input") {
	reset_cfg_dir();
	fs::create_directories("CLIENT_SETTING");
	std::ofstream("CLIENT_SETTING/ip_port.txt") << "foo:bar\n";

	// BEGIN: Borrowed code
	// https://ru.stackoverflow.com/questions/1391854/%D0%9C%D0%BE%D0%B6%D0%BD%D0%BE-%D0%BB%D0%B8-%D0%B8%D0%B7%D0%BC%D0%B5%D0%BD%D0%B8%D1%82%D1%8C-%D0%B8%D1%81%D1%82%D0%BE%D1%87%D0%BD%D0%B8%D0%BA-%D0%B2%D0%B2%D0%BE%D0%B4-cin

	// 1. создаём нужный поток-заглушку
	std::istringstream fake_in("8.8.8.8\n8080\n");

	// 2. временно подменяем rdbuf у std::cin
	std::streambuf* cin_backup = std::cin.rdbuf();
	std::cin.rdbuf(fake_in.rdbuf());
	// END: Borrowed code

	ServerConf cfg = get_config();

	std::cin.rdbuf(cin_backup);

	CHECK(cfg.ip == "8.8.8.8");
	CHECK(cfg.port == 8080);
	reset_cfg_dir();
}
