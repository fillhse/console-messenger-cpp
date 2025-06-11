#include "telegram_auth.h"
#include <map>
#include <random>
#include <cstdlib>
#include <ctime> 
#include <cstdio>
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>


std::string BOT_TOKEN;

void set_bot_token(const std::string& token) {
    BOT_TOKEN = token;
}

std::map<std::string, std::string> auth_codes;

std::string generate_auth_code() {
    static bool seeded = false;
    if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr))); seeded = true; }

    std::string digits = "0123456789", code;
    for (int i = 0; i < 6; ++i)               
        code += digits[std::rand() % 10];
    return code;
}

void ensure_bot_token()
{
    if (!BOT_TOKEN.empty()) return;

    namespace fs = std::filesystem;
    fs::path dir  = "SETTINGS";
    fs::path file = dir / "settings.txt";

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
    ensure_bot_token();
    std::string cmd = "curl -s \"https://api.telegram.org/bot" + BOT_TOKEN +
                      "/sendMessage?chat_id=" + chat_id +
                      "&text=Your%20authentication%20code%20is:%20" + code + "\"";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;

    std::string response;  char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) response += buf;
    int status = pclose(pipe);

    if (status == 0 && response.find("\"ok\":true") != std::string::npos) {
        auth_codes[chat_id] = code;
        return true;
    }
    return false;  
}

bool verify_auth_code(const std::string& chat_id, const std::string& code) {
    return auth_codes.count(chat_id) && auth_codes[chat_id] == code;
}
