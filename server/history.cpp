#include "history.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

std::string get_history_filename(const std::string& user1, const std::string& user2) {
	std::string u1 = user1, u2 = user2;
	if (u1 > u2)
		std::swap(u1, u2);
	return "HISTORY/history_" + u1 + "_" + u2 + ".txt";
}

void ensure_history_folder_exists() {
	if (!fs::exists("HISTORY")) {
		fs::create_directory("HISTORY");
	}
}

void append_message_to_history(const std::string& user1, const std::string& user2,
                               const std::string& message) {
	ensure_history_folder_exists();
	std::ofstream file(get_history_filename(user1, user2), std::ios::app);
	if (file) {
		file << message;
	}
}

std::string load_history_for_users(const std::string& user1, const std::string& user2) {
	ensure_history_folder_exists();
	std::ifstream file(get_history_filename(user1, user2));
	std::string line, text;
	if (file.is_open()) {
		while (std::getline(file, line)) {
			text += line + "\n";
		}
	}
	return text;
}
