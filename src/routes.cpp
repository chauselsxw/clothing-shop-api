#include "routes.h"

#include <functional>
#include <iostream>

#include "errors.h"

// ---------- помощники ----------

// Отправляет клиенту JSON с нужным HTTP-статусом
static void send_json(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

static void send_error(httplib::Response& res, int status, const std::string& message) {
    send_json(res, status, json{{"error", message}});
}

// Выполняет action и превращает любые исключения в JSON-ответ с ошибкой
static void run(httplib::Response& res, const std::function<void()>& action) {
    try {
        action();
    } catch (const ApiError& e) {
        send_error(res, e.status, e.what());  // ожидаемая ошибка: 400, 404, 409
    } catch (const std::exception& e) {
        std::cerr << "internal error: " << e.what() << "\n";
        send_error(res, 500, "internal server error");  // неожиданная ошибка
    }
}

// Берёт число из адреса вида /products/5
static int get_id(const httplib::Request& req) {
    try {
        return std::stoi(req.path_params.at("id"));
    } catch (const std::exception&) {
        throw ApiError(400, "id must be a number");
    }
}

// Превращает тело запроса из текста в JSON
static json parse_body(const httplib::Request& req) {
    json body = json::parse(req.body, nullptr, false);  // false = не бросать исключение
    if (body.is_discarded()) {
        throw ApiError(400, "request body is not valid JSON");
    }
    return body;
}

// ---------- маршруты ----------

void register_routes(httplib::Server& server, ShopService& service) {
    // ===== категории =====
    server.Get("/categories", [&service](const httplib::Request&, httplib::Response& res) {
        run(res, [&] { send_json(res, 200, json(service.get_categories())); });
    });

    server.Post("/categories", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] { send_json(res, 201, json(service.create_category(parse_body(req)))); });
    });

    // ===== товары =====
    server.Get("/products", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] {
            int category_id = 0;  // 0 = без фильтра
            if (req.has_param("category_id")) {
                try {
                    category_id = std::stoi(req.get_param_value("category_id"));
                } catch (const std::exception&) {
                    throw ApiError(400, "category_id must be a number");
                }
            }
            send_json(res, 200, json(service.get_products(category_id)));
        });
    });

    server.Get("/products/:id", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] { send_json(res, 200, json(service.get_product(get_id(req)))); });
    });

    server.Post("/products", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] { send_json(res, 201, json(service.create_product(parse_body(req)))); });
    });

    server.Patch("/products/:id", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] {
            send_json(res, 200, json(service.update_product(get_id(req), parse_body(req))));
        });
    });

    server.Delete("/products/:id", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] {
            service.delete_product(get_id(req));
            res.status = 204;  // 204 No Content - успех, тела ответа нет
        });
    });

    // ===== покупатели =====
    server.Get("/customers", [&service](const httplib::Request&, httplib::Response& res) {
        run(res, [&] { send_json(res, 200, json(service.get_customers())); });
    });

    server.Post("/customers", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] { send_json(res, 201, json(service.create_customer(parse_body(req)))); });
    });

    server.Patch("/customers/:id", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] {
            send_json(res, 200, json(service.update_customer(get_id(req), parse_body(req))));
        });
    });

    // ===== заказы =====
    server.Get("/orders", [&service](const httplib::Request&, httplib::Response& res) {
        run(res, [&] { send_json(res, 200, json(service.get_orders())); });
    });

    server.Get("/orders/:id", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] { send_json(res, 200, json(service.get_order(get_id(req)))); });
    });

    server.Post("/orders", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] { send_json(res, 201, json(service.create_order(parse_body(req)))); });
    });

    server.Patch("/orders/:id/status",
                 [&service](const httplib::Request& req, httplib::Response& res) {
                     run(res, [&] {
                         send_json(res, 200,
                                   json(service.change_order_status(get_id(req), parse_body(req))));
                     });
                 });

    server.Delete("/orders/:id", [&service](const httplib::Request& req, httplib::Response& res) {
        run(res, [&] {
            service.delete_order(get_id(req));
            res.status = 204;
        });
    });
}
