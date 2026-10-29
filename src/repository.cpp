#include "repository.h"

#include "query.h"

namespace repo {

// ---------- превращение строки таблицы в структуру (это и есть "ORM") ----------

static Category to_category(const pqxx::result& res, int i) {
    auto r = res[i];
    Category c;
    c.id = r["id"].as<int>();
    c.name = r["name"].as<std::string>();
    return c;
}

static Product to_product(const pqxx::result& res, int i) {
    auto r = res[i];
    Product p;
    p.id = r["id"].as<int>();
    p.category_id = r["category_id"].as<int>();
    p.name = r["name"].as<std::string>();
    p.size = r["size"].as<std::string>();
    p.color = r["color"].as<std::string>();
    p.price = r["price"].as<double>();
    p.stock = r["stock"].as<int>();
    return p;
}

static Customer to_customer(const pqxx::result& res, int i) {
    auto r = res[i];
    Customer c;
    c.id = r["id"].as<int>();
    c.name = r["name"].as<std::string>();
    c.email = r["email"].as<std::string>();
    // phone может быть NULL - тогда оставляем пустую строку
    if (!r["phone"].is_null()) {
        c.phone = r["phone"].as<std::string>();
    }
    return c;
}

static Order to_order(const pqxx::result& res, int i) {
    auto r = res[i];
    Order o;
    o.id = r["id"].as<int>();
    o.customer_id = r["customer_id"].as<int>();
    o.status = r["status"].as<std::string>();
    o.total = r["total"].as<double>();
    o.created_at = r["created_at"].as<std::string>();
    return o;
}

static std::vector<OrderItem> get_order_items(pqxx::work& tx, int order_id) {
    pqxx::result res = query(tx, 
        "SELECT product_id, quantity, price FROM order_items WHERE order_id = $1 ORDER BY id",
        order_id);
    std::vector<OrderItem> items;
    for (int i = 0; i < static_cast<int>(res.size()); i++) {
        OrderItem item;
        item.product_id = res[i]["product_id"].as<int>();
        item.quantity = res[i]["quantity"].as<int>();
        item.price = res[i]["price"].as<double>();
        items.push_back(item);
    }
    return items;
}

// ---------- категории ----------

std::vector<Category> get_categories(pqxx::work& tx) {
    pqxx::result res = tx.exec("SELECT id, name FROM categories ORDER BY id");
    std::vector<Category> categories;
    for (int i = 0; i < static_cast<int>(res.size()); i++) {
        categories.push_back(to_category(res, i));
    }
    return categories;
}

Category insert_category(pqxx::work& tx, const std::string& name) {
    pqxx::result res =
        query(tx, "INSERT INTO categories (name) VALUES ($1) RETURNING id, name", name);
    return to_category(res, 0);
}

bool category_exists(pqxx::work& tx, int id) {
    pqxx::result res = query(tx, "SELECT 1 FROM categories WHERE id = $1", id);
    return !res.empty();
}

// ---------- товары ----------

static const std::string PRODUCT_COLUMNS = "id, category_id, name, size, color, price, stock";

std::vector<Product> get_products(pqxx::work& tx, int category_id) {
    pqxx::result res = query(tx, 
        "SELECT " + PRODUCT_COLUMNS +
            " FROM products WHERE ($1 = 0 OR category_id = $1) ORDER BY id",
        category_id);
    std::vector<Product> products;
    for (int i = 0; i < static_cast<int>(res.size()); i++) {
        products.push_back(to_product(res, i));
    }
    return products;
}

std::optional<Product> get_product(pqxx::work& tx, int id, bool for_update) {
    std::string sql = "SELECT " + PRODUCT_COLUMNS + " FROM products WHERE id = $1";
    if (for_update) {
        sql += " FOR UPDATE";
    }
    pqxx::result res = query(tx, sql, id);
    if (res.empty()) {
        return std::nullopt;
    }
    return to_product(res, 0);
}

Product insert_product(pqxx::work& tx, const Product& product) {
    pqxx::result res = query(tx, 
        "INSERT INTO products (category_id, name, size, color, price, stock) "
        "VALUES ($1, $2, $3, $4, $5, $6) RETURNING " +
            PRODUCT_COLUMNS,
        product.category_id, product.name, product.size, product.color, product.price,
        product.stock);
    return to_product(res, 0);
}

void update_product(pqxx::work& tx, const Product& product) {
    query(tx, 
        "UPDATE products SET category_id = $2, name = $3, size = $4, color = $5, "
        "price = $6, stock = $7 WHERE id = $1",
        product.id, product.category_id, product.name, product.size, product.color,
        product.price, product.stock);
}

void delete_product(pqxx::work& tx, int id) {
    query(tx, "DELETE FROM products WHERE id = $1", id);
}

void change_stock(pqxx::work& tx, int product_id, int delta) {
    query(tx, "UPDATE products SET stock = stock + $2 WHERE id = $1", product_id, delta);
}

int count_product_in_orders(pqxx::work& tx, int product_id) {
    pqxx::result res =
        query(tx, "SELECT COUNT(*) FROM order_items WHERE product_id = $1", product_id);
    return res[0][0].as<int>();
}

// ---------- покупатели ----------

std::vector<Customer> get_customers(pqxx::work& tx) {
    pqxx::result res = tx.exec("SELECT id, name, email, phone FROM customers ORDER BY id");
    std::vector<Customer> customers;
    for (int i = 0; i < static_cast<int>(res.size()); i++) {
        customers.push_back(to_customer(res, i));
    }
    return customers;
}

std::optional<Customer> get_customer(pqxx::work& tx, int id) {
    pqxx::result res =
        query(tx, "SELECT id, name, email, phone FROM customers WHERE id = $1", id);
    if (res.empty()) {
        return std::nullopt;
    }
    return to_customer(res, 0);
}

Customer insert_customer(pqxx::work& tx, const Customer& customer) {
    // пустой телефон сохраняем как NULL
    std::optional<std::string> phone;
    if (!customer.phone.empty()) {
        phone = customer.phone;
    }
    pqxx::result res = query(tx, 
        "INSERT INTO customers (name, email, phone) VALUES ($1, $2, $3) "
        "RETURNING id, name, email, phone",
        customer.name, customer.email, phone);
    return to_customer(res, 0);
}

void update_customer(pqxx::work& tx, const Customer& customer) {
    std::optional<std::string> phone;
    if (!customer.phone.empty()) {
        phone = customer.phone;
    }
    query(tx, "UPDATE customers SET name = $2, email = $3, phone = $4 WHERE id = $1",
                   customer.id, customer.name, customer.email, phone);
}

// ---------- заказы ----------

std::vector<Order> get_orders(pqxx::work& tx) {
    pqxx::result res =
        tx.exec("SELECT id, customer_id, status, total, created_at FROM orders ORDER BY id");
    std::vector<Order> orders;
    for (int i = 0; i < static_cast<int>(res.size()); i++) {
        Order o = to_order(res, i);
        o.items = get_order_items(tx, o.id);
        orders.push_back(o);
    }
    return orders;
}

std::optional<Order> get_order(pqxx::work& tx, int id, bool for_update) {
    std::string sql =
        "SELECT id, customer_id, status, total, created_at FROM orders WHERE id = $1";
    if (for_update) {
        sql += " FOR UPDATE";
    }
    pqxx::result res = query(tx, sql, id);
    if (res.empty()) {
        return std::nullopt;
    }
    Order o = to_order(res, 0);
    o.items = get_order_items(tx, id);
    return o;
}

int insert_order(pqxx::work& tx, int customer_id, double total) {
    pqxx::result res = query(tx, 
        "INSERT INTO orders (customer_id, total) VALUES ($1, $2) RETURNING id", customer_id,
        total);
    return res[0]["id"].as<int>();
}

void insert_order_item(pqxx::work& tx, int order_id, const OrderItem& item) {
    query(tx, 
        "INSERT INTO order_items (order_id, product_id, quantity, price) "
        "VALUES ($1, $2, $3, $4)",
        order_id, item.product_id, item.quantity, item.price);
}

void set_order_status(pqxx::work& tx, int id, const std::string& status) {
    query(tx, "UPDATE orders SET status = $2 WHERE id = $1", id, status);
}

void delete_order(pqxx::work& tx, int id) {
    // позиции заказа удалятся сами (ON DELETE CASCADE в миграции 002)
    query(tx, "DELETE FROM orders WHERE id = $1", id);
}

}  // namespace repo
