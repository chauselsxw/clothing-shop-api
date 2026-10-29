#pragma once
#include <string>
#include <vector>

#include "models.h"

// Функции проверки входных данных. Возвращают текст ошибки,
// а если всё хорошо - пустую строку.
// partial = true значит PATCH: проверяем только те поля, которые прислали.
std::string validate_category(const json& body);
std::string validate_product(const json& body, bool partial);
std::string validate_customer(const json& body, bool partial);
std::string validate_order(const json& body);
std::string validate_status_change(const json& body);

bool is_valid_email(const std::string& email);
bool is_valid_status(const std::string& status);

// Бизнес-правило: из какого статуса заказа в какой можно перейти
bool can_change_status(const std::string& from, const std::string& to);

// Сумма заказа с округлением до копеек
double calculate_total(const std::vector<OrderItem>& items);
