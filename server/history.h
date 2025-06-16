/**
 * @file history.h
 * @brief Работа с историей переписки между двумя пользователями.
 *
 * Механизм:
 * - История хранится в каталоге HISTORY.
 * - Название файла истории для пары пользователей формируется
 *   лексикографически: HISTORY/<min>___<max>.txt.
 */

#ifndef HISTORY_H
#define HISTORY_H

#include <string>

/**
 * @brief Добавить сообщение в историю чата двух пользователей.
 *
 * Создаёт каталог HISTORY при необходимости и дописывает @p message
 * в файл для пары пользователей.
 *
 * @param user1 Идентификатор первого пользователя.
 * @param user2 Идентификатор второго пользователя.
 * @param message Текст сообщения, включая символ новой строки.
 */
void append_message_to_history(const std::string& user1, const std::string& user2,
                               const std::string& message);

/**
 * @brief Загрузить всю историю переписки между двумя пользователями.
 *
 * Открывает файл HISTORY/<min>___<max>.txt и возвращает его содержимое.
 *
 * @param user1 Идентификатор первого пользователя.
 * @param user2 Идентификатор второго пользователя.
 * @return Строка с полным содержимым истории; пустая строка,
 *         если файл не существует или пуст.
 */
std::string load_history_for_users(const std::string& user1, const std::string& user2);

#endif  // HISTORY_H
