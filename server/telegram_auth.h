/**
 * @file telegram_auth.h
 * @brief Интерфейс для Telegram-аутентификации: генерация, отправка и проверка кодов.
 *
 * Описание:
 * - Использует Telegram Bot API для отправки одноразовых кодов авторизации.
 * - Хранит сгенерированные коды в глобальной карте auth_codes.
 */

#ifndef TELEGRAM_AUTH_H
#define TELEGRAM_AUTH_H

#include <string>

/// Глобальный токен Telegram-бота, используется при отправке сообщений.
/**
 * Должен быть установлен до вызова send_telegram_code().
 */
extern std::string BOT_TOKEN;

/**
 * @brief Установить глобальный токен Telegram-бота.
 *
 * Сохраняет переданный @p token для последующих HTTP-запросов.
 *
 * @param token Строка токена, выданная BotFather.
 */
void set_bot_token(const std::string& token);

/**
 * @brief Сгенерировать случайный шестизначный код для авторизации.
 *
 * Код состоит из цифр [0-9] и всегда имеет длину 6 символов.
 *
 * @return Сгенерированный код (например, "042517").
 */
std::string generate_auth_code();

/**
 * @brief Отправить код авторизации через Telegram Bot API.
 *
 * Формирует и выполняет HTTP-запрос для отправки сообщения с одноразовым кодом.
 *
 * @param chat_id Идентификатор Telegram-чата получателя.
 * @param code    Шестизначный код, который будет отправлен.
 * @return true, если сообщение успешно отправлено и код сохранён;
 *         false в случае ошибки при вызове curl или неверного ответа API.
 */
bool send_telegram_code(const std::string& chat_id, const std::string& code);

/**
 * @brief Проверить введённый пользователем код авторизации.
 *
 * Сравнивает переданный @p code с сохранённым в auth_codes для данного @p chat_id.
 *
 * @param chat_id Идентификатор Telegram-чата, для которого код генерировался.
 * @param code    Код, введённый пользователем.
 * @return true, если код совпадает с ранее сгенерированным; иначе false.
 */
bool verify_auth_code(const std::string& chat_id, const std::string& code);

/**
 * @brief Убедиться, что глобальный токен бота загружен.
 *
 * При первом вызове пытается считать токен из файла SERVER_SETTINGS/BOT_TOKEN.txt.
 * Если файл отсутствует или пуст, создаёт его и выводит сообщение об ошибке.
 * Завершает программу при отсутствии токена.
 */
void ensure_bot_token();

#endif  // TELEGRAM_AUTH_H
