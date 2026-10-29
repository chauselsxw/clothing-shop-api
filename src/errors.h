#pragma once
#include <stdexcept>
#include <string>

// Наша ошибка для API: кроме текста хранит HTTP-код (400, 404, 409 ...).
// Сервис бросает её, а слой маршрутов превращает в JSON-ответ.
struct ApiError : public std::runtime_error {
    int status;

    ApiError(int status, const std::string& message)
        : std::runtime_error(message), status(status) {}
};
