#ifndef HISTORY_H
#define HISTORY_H

#include <string>

void append_message_to_history(const std::string& user1, const std::string& user2,
                               const std::string& message);
std::string load_history_for_users(const std::string& user1, const std::string& user2);

#endif
