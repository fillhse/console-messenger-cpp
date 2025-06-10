#include "telegram_auth.h"
#include <map>
#include <random>
#include <cstdlib>
#include <ctime> 
#include <cstdio>
#include <iostream>


//Telegram Bot Token
const std::string BOT_TOKEN = "7576904203:AAEhPFqiFgeK0cBV_nEfqVpYqShfCz60SOw";

std::map<std::string, std::string> auth_codes;

std::string generate_auth_code() {
    static bool seeded = false;
    if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr))); seeded = true; }

    std::string digits = "0123456789", code;
    for (int i = 0; i < 6; ++i)               
        code += digits[std::rand() % 10];
    return code;
}

bool send_telegram_code(const std::string& chat_id, const std::string& code) {
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
