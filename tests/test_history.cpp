#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../server/history.h"
#include "doctest/doctest.h"
#include <filesystem>

namespace fs = std::filesystem;

// Static object declaration at namespace scope:
//  - Its constructor runs before entering main(), executing backup logic
//  - Its destructor runs after exiting main(), restoring state
static struct HistoryBackup {
	HistoryBackup() {
		fs::remove_all("HISTORY.bak");
		if (fs::exists("HISTORY"))
			fs::copy("HISTORY", "HISTORY.bak", fs::copy_options::recursive);
	}
	~HistoryBackup() {
		fs::remove_all("HISTORY");
		if (fs::exists("HISTORY.bak"))
			fs::rename("HISTORY.bak", "HISTORY");
	}
} historyBackup;

TEST_SUITE("history") {
	TEST_CASE("append + load basic") {
		fs::remove_all("HISTORY");

		append_message_to_history("123", "456", "Hello");
		append_message_to_history("456", "123", "Hi!");
		auto txt = load_history_for_users("123", "456");

		CHECK(txt.find("Hello") != std::string::npos);
		CHECK(txt.find("Hi!") != std::string::npos);
	}

	TEST_CASE("order of IDs irrelevant") {
		fs::remove_all("HISTORY");

		append_message_to_history("123", "456", "Ping");
		auto a = load_history_for_users("123", "456");
		auto b = load_history_for_users("456", "123");

		CHECK(a == b);
	}

	TEST_CASE("empty history → empty string") {
		fs::remove_all("HISTORY");
		CHECK(load_history_for_users("999", "888").empty());
	}

	TEST_CASE("separate chats don’t mix") {
		fs::remove_all("HISTORY");

		append_message_to_history("123", "456", "msg_1");
		append_message_to_history("777", "888", "msg_2");

		CHECK(load_history_for_users("123", "456").find("msg_2") == std::string::npos);
		CHECK(load_history_for_users("777", "888").find("msg_1") == std::string::npos);
	}

	TEST_CASE("file created on disk") {
		fs::remove_all("HISTORY");

		append_message_to_history("123", "456", "Hi");

		const fs::path expected = "HISTORY/history_123_456.txt";
		CHECK(fs::exists(expected));
	}
}
