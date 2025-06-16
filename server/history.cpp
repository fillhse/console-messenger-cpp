/**
 * @file history.cpp
 * @brief Реализация функций для хранения и загрузки истории переписки пользователей.
 *
 * Механизм:
 * - История для пары пользователей хранится в файле HISTORY/history_<min>_<max>.txt,
 *   где <min> и <max> — идентификаторы пользователей в лексикографическом порядке.
 * - При добавлении сообщения, папка HISTORY создаётся при необходимости,
 *   а сообщение дописывается в соответствующий файл.
 * - При загрузке истории возвращается содержимое файла целиком или пустая строка,
 *   если файл не существует.
 */

#include "history.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

/**
 * @brief Построить путь к файлу истории для двух пользователей.
 *
 * Идентификаторы упорядочиваются лексикографически и соединяются
 * через '_', а затем добавляются префикс и суффикс.
 *
 * @param user1 Идентификатор первого пользователя.
 * @param user2 Идентификатор второго пользователя.
 * @return Строка с путём к файлу истории, например "HISTORY/history_alice_bob.txt".
 */
std::string get_history_filename(const std::string& user1, const std::string& user2) {
	std::string u1 = user1, u2 = user2;
	if (u1 > u2)
		std::swap(u1, u2);
	return "HISTORY/history_" + u1 + "_" + u2 + ".txt";
}

/**
 * @brief Создать папку HISTORY, если она ещё не существует.
 *
 * Использует std::filesystem::exists и std::filesystem::create_directory.
 */
void ensure_history_folder_exists() {
	if (!fs::exists("HISTORY")) {
		fs::create_directory("HISTORY");
	}
}

/**
 * @brief Добавить сообщение в конец файла истории для двух пользователей.
 *
 * Перед записью гарантирует существование папки HISTORY.
 *
 * @param user1  Идентификатор отправителя или просто один из пользователей чата.
 * @param user2  Идентификатор второго пользователя чата.
 * @param message Текст сообщения; может содержать символ новой строки в конце.
 */
void append_message_to_history(const std::string& user1, const std::string& user2,
                               const std::string& message) {
	ensure_history_folder_exists();
	std::ofstream file(get_history_filename(user1, user2), std::ios::app);
	if (file) {
		file << message;
	}
}

/**
 * @brief Загрузить всю историю переписки между двумя пользователями.
 *
 * Если файл с историей существует, читает его содержимое через std::ostringstream.
 *
 * @param user1 Идентификатор первого пользователя.
 * @param user2 Идентификатор второго пользователя.
 * @return Строка с полным содержимым файла истории; пустая строка, если файл недоступен.
 */
std::string load_history_for_users(const std::string& user1, const std::string& user2) {
	ensure_history_folder_exists();
	std::ifstream file(get_history_filename(user1, user2));
	std::ostringstream ss;
	if (file) {
		ss << file.rdbuf();
	}
	return ss.str();
}
