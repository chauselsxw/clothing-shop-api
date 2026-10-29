#pragma once
#include <string>
#include <vector>

#include "models.h"

// Сервис - бизнес-логика магазина. Здесь проверяются данные и правила,
// открываются транзакции и вызывается репозиторий.
// При ошибке бросает ApiError (см. errors.h).
class ShopService {
public:
    explicit ShopService(std::string conn_string);

    // категории
    std::vector<Category> get_categories();
    Category create_category(const json& body);

    // товары
    std::vector<Product> get_products(int category_id);
    Product get_product(int id);
    Product create_product(const json& body);
    Product update_product(int id, const json& body);
    void delete_product(int id);

    // покупатели
    std::vector<Customer> get_customers();
    Customer create_customer(const json& body);
    Customer update_customer(int id, const json& body);

    // заказы
    std::vector<Order> get_orders();
    Order get_order(int id);
    Order create_order(const json& body);
    Order change_order_status(int id, const json& body);
    void delete_order(int id);

private:
    std::string conn_string_;
};
