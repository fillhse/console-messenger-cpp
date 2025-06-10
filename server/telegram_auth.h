#ifndef TELEGRAM_AUTH_H
#define TELEGRAM_AUTH_H

#include <string>

std::string generate_auth_code();
bool send_telegram_code(const std::string& chat_id, const std::string& code);
bool verify_auth_code(const std::string& chat_id, const std::string& code);


extern std::string BOT_TOKEN;  // задаётся при запуске сервера
void set_bot_token(const std::string& token);
#endif // TELEGRAM_AUTH_H
