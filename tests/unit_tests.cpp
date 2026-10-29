// Unit-тесты: проверяют отдельные функции из rules.cpp. База данных не нужна.
#include <gtest/gtest.h>

#include "rules.h"

TEST(ProductValidation, AcceptsCorrectProduct) {
    json body = {{"name", "T-shirt"}, {"size", "M"},     {"color", "white"},
                 {"price", 1500.5},   {"stock", 10},     {"category_id", 1}};
    EXPECT_EQ(validate_product(body, false), "");
}

TEST(ProductValidation, RejectsNegativePrice) {
    json body = {{"name", "T-shirt"}, {"size", "M"},     {"color", "white"},
                 {"price", -5},       {"stock", 10},     {"category_id", 1}};
    EXPECT_EQ(validate_product(body, false), "price must be a number greater than 0");
}

TEST(ProductValidation, PatchChecksOnlyGivenFields) {
    EXPECT_EQ(validate_product(json{{"stock", 3}}, true), "");   // одно поле - нормально
    EXPECT_NE(validate_product(json::object(), true), "");        // пустой PATCH - ошибка
    EXPECT_NE(validate_product(json{{"stock", -1}}, true), "");   // плохое значение - ошибка
}

TEST(CustomerValidation, ChecksEmailFormat) {
    EXPECT_TRUE(is_valid_email("ivan@mail.ru"));
    EXPECT_FALSE(is_valid_email("ivan.mail.ru"));    // нет @
    EXPECT_FALSE(is_valid_email("ivan@mailru"));     // нет точки после @
    EXPECT_FALSE(is_valid_email("@mail.ru"));        // ничего перед @
}

TEST(OrderValidation, RejectsEmptyItemsAndZeroQuantity) {
    json empty_items = {{"customer_id", 1}, {"items", json::array()}};
    EXPECT_NE(validate_order(empty_items), "");

    json zero_quantity = {{"customer_id", 1},
                          {"items", {{{"product_id", 1}, {"quantity", 0}}}}};
    EXPECT_NE(validate_order(zero_quantity), "");

    json good = {{"customer_id", 1}, {"items", {{{"product_id", 1}, {"quantity", 2}}}}};
    EXPECT_EQ(validate_order(good), "");
}

TEST(OrderStatus, AllowsOnlyCorrectTransitions) {
    EXPECT_TRUE(can_change_status("new", "paid"));
    EXPECT_TRUE(can_change_status("paid", "shipped"));
    EXPECT_TRUE(can_change_status("shipped", "delivered"));
    EXPECT_TRUE(can_change_status("new", "cancelled"));
    EXPECT_FALSE(can_change_status("new", "shipped"));        // нельзя перепрыгнуть
    EXPECT_FALSE(can_change_status("shipped", "cancelled"));  // отправленный не отменить
    EXPECT_FALSE(can_change_status("delivered", "new"));      // конечный статус
    EXPECT_FALSE(can_change_status("cancelled", "paid"));
}

TEST(OrderTotal, SumsItemsAndRoundsToCents) {
    std::vector<OrderItem> items = {{1, 2, 1999.99}, {2, 1, 500.0}};
    EXPECT_DOUBLE_EQ(calculate_total(items), 4499.98);
    EXPECT_DOUBLE_EQ(calculate_total({}), 0.0);
}
