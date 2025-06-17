/**
 * @file socket_utils.h
 * @brief Обёртки функций отправки и приёма данных по TCP-сокетам.
 *
 * Содержит inline-функции:
 *  - send_all: отправить весь буфер данных;
 *  - send_packet: отправить пакет строки с маркером конца сообщения "*ENDM*";
 *  - send_line: отправить одну строку с терминатором '\n';
 *  - recv_line: получить одну строку до символа '\n'.
 */

#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <string_view>

/**
 * @brief Отправить весь буфер данных через сокет.
 *
 * Использует ::send() в цикле, пока не отправит все байты из @p data.
 *
 * @param fd Дескриптор сокета.
 * @param data Буфер данных для отправки.
 * @return true если все данные успешно отправлены, false при ошибке.
 */
inline bool send_all(int fd, std::string_view data) {
	std::string message(data);
	size_t sent = 0;
	while (sent < message.size()) {
		ssize_t n = ::send(fd, message.data() + sent, message.size() - sent, 0);
		if (n <= 0)
			return false;
		sent += static_cast<size_t>(n);  // Borrowed line
	}
	return true;
}

/**
 * @brief Отправить пакет текстовых данных через сокет.
 *
 * Добавляет '\n' в конец сообщения, если его нет,
 * затем добавляет маркер "*ENDM*\n" и отправляет через send_all().
 *
 * @param fd Дескриптор сокета.
 * @param data Текст данных для отправки.
 * @return true если пакет успешно отправлен, false при ошибке.
 */
inline bool send_packet(int fd, std::string_view data) {
	std::string message(data);
	if (message.empty() || message.back() != '\n')
		message.push_back('\n');
	message += "*ENDM*\n";
	return send_all(fd, message);
}

/**
 * @brief Отправить одну строку через сокет.
 *
 * Гарантирует наличие символа новой строки '\n' в конце
 * и отправляет через send_all().
 *
 * @param fd Дескриптор сокета.
 * @param sv Строка для отправки.
 * @return true если строка успешно отправлена, false при ошибке.
 */
inline bool send_line(int fd, std::string_view sv) {
	if (sv.empty() || sv.back() != '\n') {
		std::string tmp(sv);
		tmp.push_back('\n');
		return send_all(fd, tmp);
	}
	return send_all(fd, sv);
}

/**
 * @brief Прочитать одну строку из сокета до символа новой строки.
 *
 * Читает по одному символу через ::recv() и сохраняет
 * их в @p out до встречи '\n'. Символ '\n' не включается.
 *
 * @param fd Дескриптор сокета.
 * @param out Переменная для сохранения прочитанной строки.
 * @return true если строка успешно прочитана, false при закрытии соединения или ошибке.
 */
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

#endif  // SOCKET_UTILS_H
