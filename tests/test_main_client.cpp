#define main main_client_entry
#include "../client/main_client.cpp"
#undef main

#include "doctest/doctest.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

static void reset_cfg_dir() {
	fs::remove_all("CLIENT_SETTING");
}

TEST_SUITE("main_client") {
	TEST_CASE("valid_ip_port basic") {
		CHECK(valid_ip_port("127.0.0.1", 80));
		CHECK_FALSE(valid_ip_port("256.0.0.1", 80));
		CHECK_FALSE(valid_ip_port("8.8.8.8", 70000));
	}

	struct CfgFixture {
		CfgFixture() { reset_cfg_dir(); }
		~CfgFixture() { reset_cfg_dir(); }
	};

	TEST_CASE_FIXTURE(CfgFixture, "get_config returns existing valid file") {
		fs::create_directories("CLIENT_SETTING");
		std::ofstream("CLIENT_SETTING/ip_port.txt") << "127.0.0.1:12345\n";

		// redirect cin not needed here, but keep backup
		std::streambuf* cin_backup = std::cin.rdbuf();
		std::istringstream dummy("\n");
		std::cin.rdbuf(dummy.rdbuf());

		ServerConf cfg = get_config();
		std::cin.rdbuf(cin_backup);

		CHECK(cfg.ip == "127.0.0.1");
		CHECK(cfg.port == 12345);
	}

	TEST_CASE_FIXTURE(CfgFixture, "get_config reparses invalid file and saves user input") {
		fs::create_directories("CLIENT_SETTING");
		std::ofstream("CLIENT_SETTING/ip_port.txt") << "foo:bar\n";  // invalid

		// supply valid ip and port via fake cin
		std::streambuf* cin_backup = std::cin.rdbuf();
		std::istringstream fake_input("8.8.8.8\n8080\n");
		std::cin.rdbuf(fake_input.rdbuf());

		ServerConf cfg = get_config();
		std::cin.rdbuf(cin_backup);

		CHECK(cfg.ip == "8.8.8.8");
		CHECK(cfg.port == 8080);

		std::ifstream fin("CLIENT_SETTING/ip_port.txt");
		std::string line;
		std::getline(fin, line);
		CHECK(line == "8.8.8.8:8080");
	}
}
