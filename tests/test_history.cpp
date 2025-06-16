#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../server/history.h"
#include "doctest/doctest.h"
#include <filesystem>

namespace fs = std::filesystem;

TEST_SUITE("history") {
	struct Cleaner {
		Cleaner() { fs::remove_all("HISTORY"); }
		~Cleaner() { fs::remove_all("HISTORY"); }
	} _;

	TEST_CASE("append + load basic") {
		append_message_to_history("123", "456", "Hello");
		append_message_to_history("456", "123", "Hi!");
		auto txt = load_history_for_users("123", "456");
		CHECK(txt.find("Hello") != std::string::npos);
		CHECK(txt.find("Hi!") != std::string::npos);
	}

	TEST_CASE("order of IDs irrelevant") {
		append_message_to_history("123", "456", "Ping");
		CHECK(load_history_for_users("123", "456") == load_history_for_users("456", "123"));
	}

	TEST_CASE("empty history → empty string") {
		CHECK(load_history_for_users("999", "888").empty());
	}

	TEST_CASE("separate chats don’t mix") {
		append_message_to_history("123", "456", "msg_1");
		append_message_to_history("777", "888", "msg_2");

		CHECK(load_history_for_users("123", "456").find("msg_2") == std::string::npos);
		CHECK(load_history_for_users("777", "888").find("msg_1") == std::string::npos);
	}

	TEST_CASE("file created on disk") {
		append_message_to_history("123", "456", "Hi");

		bool found = false;
		if (fs::exists("HISTORY"))
			for (auto const& p : fs::directory_iterator("HISTORY")) {
				auto name = p.path().filename().string();
				if (name.find("123") != std::string::npos && name.find("456") != std::string::npos) {
					found = true;
					break;
				}
			}
		CHECK(found);
	}
}
