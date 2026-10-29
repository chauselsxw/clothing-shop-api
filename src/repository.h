#pragma once
#include <optional>
#include <string>
#include <vector>

#include <pqxx/pqxx>

#include "models.h"

// Репозиторий - единственное место, где пишется SQL.
// Все функции принимают открытую транзакцию tx; commit делает вызывающий код (сервис).
namespace repo {

// ----- категории -----
std::vector<Category> get_categories(pqxx::work& tx);
Category insert_category(pqxx::work& tx, const std::string& name);
bool category_exists(pqxx::work& tx, int id);

// ----- товары -----
// category_id = 0 значит "все категории"
std::vector<Product> get_products(pqxx::work& tx, int category_id);
// for_update = true блокирует строку до конца транзакции (защита от гонки при заказах)
std::optional<Product> get_product(pqxx::work& tx, int id, bool for_update = false);
Product insert_product(pqxx::work& tx, const Product& product);
void update_product(pqxx::work& tx, const Product& product);
void delete_product(pqxx::work& tx, int id);
// прибавляет delta к остатку на складе (delta может быть отрицательной)
void change_stock(pqxx::work& tx, int product_id, int delta);
int count_product_in_orders(pqxx::work& tx, int product_id);

// ----- покупатели -----
std::vector<Customer> get_customers(pqxx::work& tx);
std::optional<Customer> get_customer(pqxx::work& tx, int id);
Customer insert_customer(pqxx::work& tx, const Customer& customer);
void update_customer(pqxx::work& tx, const Customer& customer);

// ----- заказы -----
std::vector<Order> get_orders(pqxx::work& tx);
std::optional<Order> get_order(pqxx::work& tx, int id, bool for_update = false);
int insert_order(pqxx::work& tx, int customer_id, double total);
void insert_order_item(pqxx::work& tx, int order_id, const OrderItem& item);
void set_order_status(pqxx::work& tx, int id, const std::string& status);
void delete_order(pqxx::work& tx, int id);

}  // namespace repo
