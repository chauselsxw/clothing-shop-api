#include <iostream>

#include "config.h"
#include "migrations.h"
#include "routes.h"
#include "service.h"

int main() {
    try {
        Config cfg = load_config();
        std::string conn_string = make_conn_string(cfg, cfg.db_name);

        // при каждом запуске применяем новые миграции (уже применённые пропускаются)
        run_migrations(conn_string, cfg.migrations_dir);

        ShopService service(conn_string);
        httplib::Server server;
        register_routes(server, service);

        std::cout << "Server is running on http://localhost:" << cfg.server_port << std::endl;
        if (!server.listen("0.0.0.0", cfg.server_port)) {
            std::cerr << "Cannot start server (is the port already in use?)" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
