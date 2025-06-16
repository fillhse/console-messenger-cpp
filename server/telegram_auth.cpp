/**
 * @file telegram_auth.cpp
 * @brief Реализация функций Telegram-аутентификации: генерация, отправка и проверка кодов.
 *
 * Использует CURL (через системный вызов curl) для отправки сообщений
 * Telegram Bot API. Хранит временные коды в глобальной карте auth_codes.
 */

#include "telegram_auth.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>

// Глобальная переменная для хранения токена Telegram-бота.
std::string BOT_TOKEN;

//------------------------------------------------------------------------------

/**
 * @brief Установить глобальный токен бота.
 *
 * Сохраняет переданный токен в переменную BOT_TOKEN.
 *
 * @param token Токен бота, выданный BotFather.
 */
void set_bot_token(const std::string& token) {
	BOT_TOKEN = token;
}

//------------------------------------------------------------------------------

/// Глобальная карта, хранящая соответствие chat_id -> код авторизации.
std::map<std::string, std::string> auth_codes;

//------------------------------------------------------------------------------

/**
 * @brief Сгенерировать случайный шестизначный код для авторизации.
 *
 * Использует std::rand(), инициализирует генератор при первом вызове
 * на основе текущего времени.
 *
 * @return Сгенерированный код (строка из 6 цифр).
 */
std::string generate_auth_code() {
	static bool seeded = false;
	if (!seeded) {
		std::srand(static_cast<unsigned>(std::time(nullptr)));
		seeded = true;
	}

	std::string digits = "0123456789", code;
	for (int i = 0; i < 6; ++i)
		code += digits[std::rand() % 10];
	return code;
}

//------------------------------------------------------------------------------

/**
 * @brief Убедиться, что токен бота загружен из файла.
 *
 * Если переменная BOT_TOKEN пуста, пытается считать токен из файла
 * SERVER_SETTINGS/BOT_TOKEN.txt. При отсутствии директории или файла
 * создаёт их. Если после чтения токен остаётся пустым — выводит
 * сообщение об ошибке и завершает программу.
 */
void ensure_bot_token() {
	if (!BOT_TOKEN.empty())
		return;

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

//------------------------------------------------------------------------------

/**
 * @brief Отправить код авторизации через Telegram Bot API.
 *
 * Формирует HTTP запрос с помощью системного вызова curl и отправляет код
 * пользователю в чат.
 *
 * @param chat_id Идентификатор Telegram-чата.
 * @param code    Шестизначный код авторизации.
 * @return true если запрос выполнен успешно (ответ содержит "ok":true),
 *         и код сохранён в auth_codes; иначе false.
 */
bool send_telegram_code(const std::string& chat_id, const std::string& code) {
	ensure_bot_token();
	std::string url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage";
	std::string cmd = "curl -s -X POST \"" + url +
	                  "\""
	                  " -d chat_id=" +
	                  chat_id + " -d text='Your authentication code is: " + code + "'";

	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe)
		return false;

	std::string response;
	char buf[256];
	while (fgets(buf, sizeof(buf), pipe))
		response += buf;
	int status = pclose(pipe);

	if (status == 0 && response.find("\"ok\":true") != std::string::npos) {
		auth_codes[chat_id] = code;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------

/**
 * @brief Проверить введённый код авторизации.
 *
 * Сравнивает переданный @p code с сохранённым в auth_codes для данного @p chat_id.
 *
 * @param chat_id Идентификатор Telegram-чата.
 * @param code    Введённый пользователем код.
 * @return true если код корректен и сохранённая запись совпадает; иначе false.
 */
bool verify_auth_code(const std::string& chat_id, const std::string& code) {
	return auth_codes.count(chat_id) && auth_codes[chat_id] == code;
}
