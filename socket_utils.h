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

/**
 * @brief  Отправить всю строку целиком по TCP-сокету.
 *
 * Функция многократно вызывает системный ::send(), пока не
 * будет передан каждый байт строки @p msg.  Рассчитана на
 * **блокирующий** сокет — ::send() внутри может подождать,
 * когда освободится буфер ядра.
 *
 * @param fd   Дескриптор открытого TCP-сокета.
 * @param msg  Строка, которую нужно передать (без копирования —
 *             используется её внутренний буфер).
 *
 * @return true   Если переданы все символы строки.
 * @return false  Если ::send() вернул 0 (соединение закрыто)
 *                или < 0 (критическая ошибка).
 */
inline bool send_all(
    int fd, const std::string& msg) {  // Без const ссылка на временный std::string запрещена стандартом.
	size_t sent = 0;
	while (sent < msg.size()) {
		ssize_t n = ::send(fd,
		                   msg.c_str() + sent,  // адрес нужного байта
		                   msg.size() - sent, 0);
		if (n <= 0)  // ошибка или разрыв
			return false;
		sent += static_cast<size_t>(n);
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
inline bool send_packet(int fd, std::string message) {
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
inline bool send_line(int fd, std::string message) {
	if (message.empty() || message.back() != '\n') {
		message.push_back('\n');
	}
	return send_all(fd, message);
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
