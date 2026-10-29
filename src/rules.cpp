#include "rules.h"

#include <cmath>

// ---------- маленькие помощники (видны только в этом файле) ----------

// Поле есть, это непустая строка и она не длиннее max_len
static bool is_text(const json& body, const std::string& key, size_t max_len) {
    if (!body.contains(key) || !body[key].is_string()) {
        return false;
    }
    std::string value = body[key].get<std::string>();
    return !value.empty() && value.size() <= max_len;
}

// Поле есть, это целое число и оно лежит между min_value и max_value
static bool is_int_between(const json& body, const std::string& key, long long min_value,
                           long long max_value) {
    if (!body.contains(key) || !body[key].is_number_integer()) {
        return false;
    }
    long long value = body[key].get<long long>();
    return value >= min_value && value <= max_value;
}

// Поле есть, это число, и оно больше 0
static bool is_price(const json& body, const std::string& key) {
    if (!body.contains(key) || !body[key].is_number()) {
        return false;
    }
    double value = body[key].get<double>();
    return value > 0 && value <= 1000000;
}

// ---------- проверка email, статусов ----------

bool is_valid_email(const std::string& email) {
    size_t at = email.find('@');
    if (at == std::string::npos || at == 0) {
        return false;  // нет символа @ или он стоит первым
    }
    size_t dot = email.find('.', at);
    if (dot == std::string::npos || dot == at + 1 || dot == email.size() - 1) {
        return false;  // после @ нет точки, или она сразу после @, или в самом конце
    }
    return email.find(' ') == std::string::npos;  // пробелов быть не должно
}

bool is_valid_status(const std::string& status) {
    return status == "new" || status == "paid" || status == "shipped" || status == "delivered" ||
           status == "cancelled";
}

// ---------- валидация тел запросов ----------

std::string validate_category(const json& body) {
    if (!body.is_object()) {
        return "body must be a JSON object";
    }
    if (!is_text(body, "name", 100)) {
        return "name must be a non-empty string (max 100 chars)";
    }
    return "";
}

std::string validate_product(const json& body, bool partial) {
    if (!body.is_object()) {
        return "body must be a JSON object";
    }
    if (partial && body.empty()) {
        return "nothing to update";
    }
    // Для POST проверяем все поля, для PATCH - только присланные
    if (!partial || body.contains("name")) {
        if (!is_text(body, "name", 100)) {
            return "name must be a non-empty string (max 100 chars)";
        }
    }
    if (!partial || body.contains("size")) {
        if (!is_text(body, "size", 10)) {
            return "size must be a non-empty string (max 10 chars)";
        }
    }
    if (!partial || body.contains("color")) {
        if (!is_text(body, "color", 30)) {
            return "color must be a non-empty string (max 30 chars)";
        }
    }
    if (!partial || body.contains("price")) {
        if (!is_price(body, "price")) {
            return "price must be a number greater than 0";
        }
    }
    if (!partial || body.contains("stock")) {
        if (!is_int_between(body, "stock", 0, 1000000)) {
            return "stock must be an integer from 0 to 1000000";
        }
    }
    if (!partial || body.contains("category_id")) {
        if (!is_int_between(body, "category_id", 1, 2000000000)) {
            return "category_id must be a positive integer";
        }
    }
    return "";
}

std::string validate_customer(const json& body, bool partial) {
    if (!body.is_object()) {
        return "body must be a JSON object";
    }
    if (partial && body.empty()) {
        return "nothing to update";
    }
    if (!partial || body.contains("name")) {
        if (!is_text(body, "name", 100)) {
            return "name must be a non-empty string (max 100 chars)";
        }
    }
    if (!partial || body.contains("email")) {
        if (!is_text(body, "email", 100) || !is_valid_email(body["email"].get<std::string>())) {
            return "email must be a valid address";
        }
    }
    // телефон необязателен, но если он есть - это строка до 20 символов
    if (body.contains("phone")) {
        if (!body["phone"].is_string() || body["phone"].get<std::string>().size() > 20) {
            return "phone must be a string (max 20 chars)";
        }
    }
    return "";
}

std::string validate_order(const json& body) {
    if (!body.is_object()) {
        return "body must be a JSON object";
    }
    if (!is_int_between(body, "customer_id", 1, 2000000000)) {
        return "customer_id must be a positive integer";
    }
    if (!body.contains("items") || !body["items"].is_array() || body["items"].empty()) {
        return "items must be a non-empty array";
    }
    for (const auto& item : body["items"]) {
        if (!item.is_object() || !is_int_between(item, "product_id", 1, 2000000000)) {
            return "each item needs a positive integer product_id";
        }
        if (!is_int_between(item, "quantity", 1, 1000000)) {
            return "each item needs quantity from 1 to 1000000";
        }
    }
    return "";
}

std::string validate_status_change(const json& body) {
    if (!body.is_object() || !body.contains("status") || !body["status"].is_string() ||
        !is_valid_status(body["status"].get<std::string>())) {
        return "status must be one of: new, paid, shipped, delivered, cancelled";
    }
    return "";
}

// ---------- бизнес-правила ----------

bool can_change_status(const std::string& from, const std::string& to) {
    if (from == "new") {
        return to == "paid" || to == "cancelled";
    }
    if (from == "paid") {
        return to == "shipped" || to == "cancelled";
    }
    if (from == "shipped") {
        return to == "delivered";
    }
    return false;  // delivered и cancelled - конечные статусы
}

double calculate_total(const std::vector<OrderItem>& items) {
    double sum = 0;
    for (const auto& item : items) {
        sum += item.price * item.quantity;
    }
    return std::round(sum * 100) / 100;  // округляем до 2 знаков
}
