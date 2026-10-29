// Integration-тесты: работают с настоящей PostgreSQL (тестовой базой TEST_DB_NAME из .env).
// Проверяют, что сервис, репозиторий, SQL и миграции работают вместе.
#include <gtest/gtest.h>

#include <thread>

#include <pqxx/pqxx>

#include "config.h"
#include "errors.h"
#include "migrations.h"
#include "routes.h"
#include "service.h"

class IntegrationTest : public ::testing::Test {
protected:
    static std::string conn_string;

    // Один раз перед всеми тестами: применяем миграции к тестовой базе
    static void SetUpTestSuite() {
        Config cfg = load_config();
        if (cfg.test_db_name == cfg.db_name) {
            throw std::runtime_error("TEST_DB_NAME must differ from DB_NAME");
        }
        conn_string = make_conn_string(cfg, cfg.test_db_name);
        run_migrations(conn_string, cfg.migrations_dir);
    }

    // Перед каждым тестом: очищаем все таблицы
    void SetUp() override {
        pqxx::connection conn(conn_string);
        pqxx::work tx(conn);
        tx.exec("TRUNCATE order_items, orders, products, categories, customers RESTART IDENTITY CASCADE");
        tx.commit();
    }

    // Создаёт категорию, товар с остатком stock и покупателя. Возвращает id товара.
    int create_product_and_customer(ShopService& service, int stock) {
        Category category = service.create_category(json{{"name", "T-shirts"}});
        Product product = service.create_product(json{{"name", "White T-shirt"},
                                                      {"size", "M"},
                                                      {"color", "white"},
                                                      {"price", 1000},
                                                      {"stock", stock},
                                                      {"category_id", category.id}});
        service.create_customer(json{{"name", "Ivan"}, {"email", "ivan@mail.ru"}});
        return product.id;
    }
};

std::string IntegrationTest::conn_string;

TEST_F(IntegrationTest, CreatedProductCanBeReadBack) {
    ShopService service(conn_string);
    int product_id = create_product_and_customer(service, 5);

    Product product = service.get_product(product_id);
    EXPECT_EQ(product.name, "White T-shirt");
    EXPECT_EQ(product.stock, 5);
    EXPECT_DOUBLE_EQ(product.price, 1000);
}

TEST_F(IntegrationTest, OrderDecreasesStockAndCalculatesTotal) {
    ShopService service(conn_string);
    int product_id = create_product_and_customer(service, 5);

    Order order = service.create_order(
        json{{"customer_id", 1}, {"items", {{{"product_id", product_id}, {"quantity", 2}}}}});

    EXPECT_EQ(order.status, "new");
    EXPECT_DOUBLE_EQ(order.total, 2000);
    EXPECT_EQ(service.get_product(product_id).stock, 3);  // было 5, купили 2
}

TEST_F(IntegrationTest, OrderWithTooBigQuantityIsRejected) {
    ShopService service(conn_string);
    int product_id = create_product_and_customer(service, 2);

    try {
        service.create_order(
            json{{"customer_id", 1}, {"items", {{{"product_id", product_id}, {"quantity", 3}}}}});
        FAIL() << "expected ApiError";
    } catch (const ApiError& e) {
        EXPECT_EQ(e.status, 409);
    }
    EXPECT_EQ(service.get_product(product_id).stock, 2);  // остаток не изменился
    EXPECT_TRUE(service.get_orders().empty());            // заказ не создан
}

TEST_F(IntegrationTest, CancelledOrderReturnsStock) {
    ShopService service(conn_string);
    int product_id = create_product_and_customer(service, 5);
    Order order = service.create_order(
        json{{"customer_id", 1}, {"items", {{{"product_id", product_id}, {"quantity", 4}}}}});
    ASSERT_EQ(service.get_product(product_id).stock, 1);

    service.change_order_status(order.id, json{{"status", "cancelled"}});

    EXPECT_EQ(service.get_product(product_id).stock, 5);  // товары вернулись на склад
    EXPECT_EQ(service.get_order(order.id).status, "cancelled");
}

TEST_F(IntegrationTest, ProductFromOrderCannotBeDeleted) {
    ShopService service(conn_string);
    int product_id = create_product_and_customer(service, 5);
    service.create_order(
        json{{"customer_id", 1}, {"items", {{{"product_id", product_id}, {"quantity", 1}}}}});

    try {
        service.delete_product(product_id);
        FAIL() << "expected ApiError";
    } catch (const ApiError& e) {
        EXPECT_EQ(e.status, 409);
    }
    EXPECT_NO_THROW(service.get_product(product_id));  // товар остался на месте
}

// Полный путь: настоящий HTTP-запрос -> маршрут -> сервис -> база данных
TEST_F(IntegrationTest, HttpApiCreatesAndReturnsData) {
    ShopService service(conn_string);
    httplib::Server server;
    register_routes(server, service);
    std::thread server_thread([&] { server.listen("127.0.0.1", 18080); });
    server.wait_until_ready();

    httplib::Client client("127.0.0.1", 18080);

    auto created = client.Post("/categories", R"({"name":"Jackets"})", "application/json");
    ASSERT_TRUE(created);
    EXPECT_EQ(created->status, 201);

    auto list = client.Get("/categories");
    ASSERT_TRUE(list);
    EXPECT_EQ(list->status, 200);
    EXPECT_EQ(json::parse(list->body).size(), 1);

    auto bad = client.Post("/categories", R"({"name":""})", "application/json");
    ASSERT_TRUE(bad);
    EXPECT_EQ(bad->status, 400);  // валидация сработала

    auto missing = client.Get("/products/999");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 404);

    server.stop();
    server_thread.join();
}
