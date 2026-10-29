#include "service.h"

#include <pqxx/pqxx>

#include "errors.h"
#include "repository.h"
#include "rules.h"

ShopService::ShopService(std::string conn_string) : conn_string_(std::move(conn_string)) {}

// Во всех методах одна схема: открыли соединение и транзакцию, сделали работу,
// для изменений вызвали commit. Если по пути бросили исключение,
// транзакция откатывается сама (в деструкторе tx).

// ---------- категории ----------

std::vector<Category> ShopService::get_categories() {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    return repo::get_categories(tx);
}

Category ShopService::create_category(const json& body) {
    std::string error = validate_category(body);
    if (!error.empty()) {
        throw ApiError(400, error);
    }

    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    try {
        Category category = repo::insert_category(tx, body["name"].get<std::string>());
        tx.commit();
        return category;
    } catch (const pqxx::unique_violation&) {
        throw ApiError(409, "category with this name already exists");
    }
}

// ---------- товары ----------

std::vector<Product> ShopService::get_products(int category_id) {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    return repo::get_products(tx, category_id);
}

Product ShopService::get_product(int id) {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    auto product = repo::get_product(tx, id);
    if (!product) {
        throw ApiError(404, "product not found");
    }
    return *product;
}

Product ShopService::create_product(const json& body) {
    std::string error = validate_product(body, false);
    if (!error.empty()) {
        throw ApiError(400, error);
    }

    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);

    Product product;
    product.category_id = body["category_id"].get<int>();
    product.name = body["name"].get<std::string>();
    product.size = body["size"].get<std::string>();
    product.color = body["color"].get<std::string>();
    product.price = body["price"].get<double>();
    product.stock = body["stock"].get<int>();

    if (!repo::category_exists(tx, product.category_id)) {
        throw ApiError(404, "category not found");
    }
    Product created = repo::insert_product(tx, product);
    tx.commit();
    return created;
}

Product ShopService::update_product(int id, const json& body) {
    std::string error = validate_product(body, true);
    if (!error.empty()) {
        throw ApiError(400, error);
    }

    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);

    auto found = repo::get_product(tx, id);
    if (!found) {
        throw ApiError(404, "product not found");
    }
    Product product = *found;

    // меняем только те поля, которые прислал клиент
    if (body.contains("category_id")) {
        product.category_id = body["category_id"].get<int>();
        if (!repo::category_exists(tx, product.category_id)) {
            throw ApiError(404, "category not found");
        }
    }
    if (body.contains("name")) {
        product.name = body["name"].get<std::string>();
    }
    if (body.contains("size")) {
        product.size = body["size"].get<std::string>();
    }
    if (body.contains("color")) {
        product.color = body["color"].get<std::string>();
    }
    if (body.contains("price")) {
        product.price = body["price"].get<double>();
    }
    if (body.contains("stock")) {
        product.stock = body["stock"].get<int>();
    }

    repo::update_product(tx, product);
    tx.commit();
    return product;
}

void ShopService::delete_product(int id) {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);

    if (!repo::get_product(tx, id)) {
        throw ApiError(404, "product not found");
    }
    // Бизнес-правило: товар, который уже есть в заказах, удалять нельзя
    if (repo::count_product_in_orders(tx, id) > 0) {
        throw ApiError(409, "product is used in orders and cannot be deleted");
    }
    repo::delete_product(tx, id);
    tx.commit();
}

// ---------- покупатели ----------

std::vector<Customer> ShopService::get_customers() {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    return repo::get_customers(tx);
}

Customer ShopService::create_customer(const json& body) {
    std::string error = validate_customer(body, false);
    if (!error.empty()) {
        throw ApiError(400, error);
    }

    Customer customer;
    customer.name = body["name"].get<std::string>();
    customer.email = body["email"].get<std::string>();
    if (body.contains("phone")) {
        customer.phone = body["phone"].get<std::string>();
    }

    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    try {
        Customer created = repo::insert_customer(tx, customer);
        tx.commit();
        return created;
    } catch (const pqxx::unique_violation&) {
        throw ApiError(409, "customer with this email already exists");
    }
}

Customer ShopService::update_customer(int id, const json& body) {
    std::string error = validate_customer(body, true);
    if (!error.empty()) {
        throw ApiError(400, error);
    }

    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);

    auto found = repo::get_customer(tx, id);
    if (!found) {
        throw ApiError(404, "customer not found");
    }
    Customer customer = *found;

    if (body.contains("name")) {
        customer.name = body["name"].get<std::string>();
    }
    if (body.contains("email")) {
        customer.email = body["email"].get<std::string>();
    }
    if (body.contains("phone")) {
        customer.phone = body["phone"].get<std::string>();
    }

    try {
        repo::update_customer(tx, customer);
        tx.commit();
        return customer;
    } catch (const pqxx::unique_violation&) {
        throw ApiError(409, "customer with this email already exists");
    }
}

// ---------- заказы ----------

std::vector<Order> ShopService::get_orders() {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    return repo::get_orders(tx);
}

Order ShopService::get_order(int id) {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);
    auto order = repo::get_order(tx, id);
    if (!order) {
        throw ApiError(404, "order not found");
    }
    return *order;
}

Order ShopService::create_order(const json& body) {
    std::string error = validate_order(body);
    if (!error.empty()) {
        throw ApiError(400, error);
    }

    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);

    int customer_id = body["customer_id"].get<int>();
    if (!repo::get_customer(tx, customer_id)) {
        throw ApiError(404, "customer not found");
    }

    std::vector<OrderItem> items;
    for (const auto& item_json : body["items"]) {
        int product_id = item_json["product_id"].get<int>();
        int quantity = item_json["quantity"].get<int>();

        // Берём товар с блокировкой строки, чтобы два заказа одновременно
        // не купили один и тот же остаток
        auto product = repo::get_product(tx, product_id, true);
        if (!product) {
            throw ApiError(404, "product " + std::to_string(product_id) + " not found");
        }
        // Бизнес-правило: нельзя заказать больше, чем есть на складе
        if (product->stock < quantity) {
            throw ApiError(409, "not enough stock for product " + std::to_string(product_id));
        }
        repo::change_stock(tx, product_id, -quantity);

        OrderItem item;
        item.product_id = product_id;
        item.quantity = quantity;
        item.price = product->price;  // запоминаем цену на момент заказа
        items.push_back(item);
    }

    int order_id = repo::insert_order(tx, customer_id, calculate_total(items));
    for (const auto& item : items) {
        repo::insert_order_item(tx, order_id, item);
    }

    Order order = *repo::get_order(tx, order_id);
    tx.commit();
    return order;
}

Order ShopService::change_order_status(int id, const json& body) {
    std::string error = validate_status_change(body);
    if (!error.empty()) {
        throw ApiError(400, error);
    }
    std::string new_status = body["status"].get<std::string>();

    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);

    auto order = repo::get_order(tx, id, true);
    if (!order) {
        throw ApiError(404, "order not found");
    }
    // Бизнес-правило: статус можно менять только по разрешённой цепочке
    if (!can_change_status(order->status, new_status)) {
        throw ApiError(409, "cannot change status from " + order->status + " to " + new_status);
    }
    // Бизнес-правило: при отмене заказа товары возвращаются на склад
    if (new_status == "cancelled") {
        for (const auto& item : order->items) {
            repo::change_stock(tx, item.product_id, item.quantity);
        }
    }

    repo::set_order_status(tx, id, new_status);
    order->status = new_status;
    tx.commit();
    return *order;
}

void ShopService::delete_order(int id) {
    pqxx::connection conn(conn_string_);
    pqxx::work tx(conn);

    auto order = repo::get_order(tx, id, true);
    if (!order) {
        throw ApiError(404, "order not found");
    }
    // Бизнес-правило: удалять можно только отменённые заказы
    if (order->status != "cancelled") {
        throw ApiError(409, "only cancelled orders can be deleted");
    }
    repo::delete_order(tx, id);
    tx.commit();
}
