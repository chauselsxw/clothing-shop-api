#pragma once
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Модели - обычные структуры. Каждая соответствует таблице в базе данных.

struct Category {
    int id = 0;
    std::string name;
};

struct Product {
    int id = 0;
    int category_id = 0;
    std::string name;
    std::string size;
    std::string color;
    double price = 0;
    int stock = 0;  // сколько штук на складе
};

struct Customer {
    int id = 0;
    std::string name;
    std::string email;
    std::string phone;  // пустая строка = телефона нет (в базе будет NULL)
};

struct OrderItem {
    int product_id = 0;
    int quantity = 0;
    double price = 0;  // цена товара на момент заказа
};

struct Order {
    int id = 0;
    int customer_id = 0;
    std::string status;  // new, paid, shipped, delivered, cancelled
    double total = 0;
    std::string created_at;
    std::vector<OrderItem> items;
};

// Макрос из библиотеки nlohmann::json: сам создаёт функции,
// которые превращают структуру в JSON (и обратно).
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Category, id, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Product, id, category_id, name, size, color, price, stock)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Customer, id, name, email, phone)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OrderItem, product_id, quantity, price)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Order, id, customer_id, status, total, created_at, items)
