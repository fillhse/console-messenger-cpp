#include "telegram_auth.h"

#include <cpr/cpr.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>

std::string BOT_TOKEN;

std::map<std::string, std::string> auth_codes;

std::string generate_auth_code() {
	static bool seeded = false;
	if (!seeded) {
		std::srand(static_cast<unsigned int>(std::time(nullptr)));
		seeded = true;
	}

	std::string digits = "0123456789", code;
	for (int i = 0; i < 6; ++i)
		code += digits[std::rand() % 10];
	return code;
}

void ensure_bot_token() {
	namespace fs = std::filesystem;
	fs::path dir = "SERVER_SETTINGS";
	fs::path file = dir / "BOT_TOKEN.txt";

	if (!fs::exists(dir))
		fs::create_directories(dir);

	if (fs::exists(file)) {
		std::ifstream in(file);
		std::getline(in, BOT_TOKEN);
	} else {
		std::ofstream out(file);
	}

	if (BOT_TOKEN.empty()) {
		std::cerr << "[TelegramAuth] Файл " << file
		          << " не содержит токен.\n"
		             "Добавьте токен в первую строку и перезапустите сервер.\n";
		exit(1);
	}
}

bool send_telegram_code(const std::string& chat_id, const std::string& code) {
	cpr::Response response = cpr::Post(cpr::Url{"https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage"},
	                                   cpr::Payload{{"chat_id", chat_id}, {"text", "Your code is: " + code}});
	if (response.text.find("\"ok\":true") != std::string::npos) {
		auth_codes[chat_id] = code;
		return true;
	}
	return false;
}

bool verify_auth_code(const std::string& chat_id, const std::string& code) {
	return auth_codes.count(chat_id) && auth_codes[chat_id] == code;
}
