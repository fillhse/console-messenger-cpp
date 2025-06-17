/**
 * @file main_client.cpp
 * @brief Клиент консольного мессенджера: подключение к серверу и обмен сообщениями.
 *
 * Программа читает конфигурацию сервера (IP и порт),
 * устанавливает TCP-соединение, запускает поток
 * для приёма сообщений и отправляет введённые пользователем строки.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket_utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

/**
 * @brief Максимально допустимая длина сообщения от пользователя.
 */
constexpr size_t MAX_INPUT = 2000;

/**
 * @brief Директория для хранения конфигурационного файла.
 */
const std::string CFG_DIR = "CLIENT_SETTING";

/**
 * @brief Путь к файлу с настройками (IP и порт сервера).
 */
const std::string CFG_FILE = "CLIENT_SETTING/ip_port.txt";

/**
 * @struct ServerConf
 * @brief Параметры подключения к серверу.
 *
 * @var ServerConf::ip   IPv4-адрес сервера.
 * @var ServerConf::port Порт сервера.
 */
struct ServerConf {
	std::string ip; /**< IPv4-адрес сервера. */
	int port;       /**< Порт сервера. */
};

/**
 * @brief Проверить корректность IPv4-адреса и порта.
 *
 * Использует inet_pton() для валидации формата IPv4
 * и проверяет, что порт находится в диапазоне 1..65535.
 *
 * @param ip    Строка с IPv4-адресом.
 * @param port  Номер порта.
 * @return true  если адрес и порт валидны;
 *         false в противном случае.
 */
bool valid_ip_port(const std::string& ip, int port) {
	sockaddr_in tmp{};
	return inet_pton(AF_INET, ip.c_str(), &tmp.sin_addr) == 1 && port > 0 && port < 65536;
}

/**
 * @brief Считать или запросить у пользователя настройки сервера.
 *
 * Если файл с конфигурацией существует, пытается прочитать из него строку
 * в формате "IP:порт". Если данные некорректны или файла нет,
 * запрашивает ввод у пользователя до тех пор, пока не будет введена
 * валидная пара.
 * Сохраняет корректные настройки в файл.
 *
 * @return Настройки сервера в виде ServerConf.
 */
ServerConf get_config() {
	std::filesystem::create_directories(CFG_DIR);
	std::ifstream fin(CFG_FILE);
	std::string ip;
	int port;
	bool ok = false;
	if (fin) {
		std::string line;
		std::getline(fin, line);
		std::istringstream ss(line);
		std::getline(ss, ip, ':');
		ss >> port;
		ok = valid_ip_port(ip, port);
	}
	while (!ok) {
		std::cout << "Enter server IP: ";
		std::cin >> ip;
		std::cout << "Enter server port: ";
		std::cin >> port;
		std::cin.ignore();
		ok = valid_ip_port(ip, port);
		if (!ok)
			std::cout << "Invalid IP or port. Try again.\n";
	}
	std::ofstream(CFG_FILE, std::ios::trunc) << ip << ':' << port << '\n';
	return {ip, port};
}

/**
 * @brief Цикл приёма и вывода сообщений от сервера.
 *
 * Читает строки из сокета через recv_line() до разрыва соединения.
 * Выводит каждую строку на консоль. При получении специального
 * маркера "*ENDM*" отображает приглашение ввода.
 *
 * @param fd Дескриптор подключённого сокета сервера.
 */
void receive_messages(int fd) {
	std::string line;
	while (recv_line(fd, line)) {
		if (line.empty())
			continue;
		if (line == "*ENDM*") {
			std::cout << "> " << std::flush;
			continue;
		}
		std::cout << line << '\n';
	}
	std::cout << "\nDisconnected from server.\n";
	close(fd);
	exit(0);
}

/**
 * @brief Точка входа клиентского приложения.
 *
 * Получает конфигурацию сервера, устанавливает TCP-соединение,
 * запускает поток для приёма сообщений и в цикле
 * отправляет введённые пользователем сообщения.
 *
 * @return Код завершения (0 при успехе, иначе 1).
 */
int main() {
	ServerConf conf = get_config();

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("socket");
		return 1;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(conf.port);
	inet_pton(AF_INET, conf.ip.c_str(), &addr.sin_addr);

	if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("connect");
		return 1;
	}

	std::thread(receive_messages, sock).detach();

	std::string input;
	while (std::getline(std::cin, input)) {
		if (input.empty())
			continue;
		if (input.size() > MAX_INPUT) {
			std::cout << "Message longer than 2000 characters. Split it.\n";
			continue;
		}
		if (input == "/exit") {
			send_line(sock, "/exit");
			std::cout << "\nExiting...\n";
			break;
		}
		send_line(sock, input);
	}
	close(sock);
	return 0;
}
