#include "config.h"

#include <fstream>
#include <map>
#include <stdexcept>

// Убирает пробелы и переводы строк по краям строки
static std::string trim(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

Config load_config() {
    // PROJECT_DIR задаётся в CMakeLists.txt и равен папке проекта
    std::string path = std::string(PROJECT_DIR) + "/.env";
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("cannot open file " + path);
    }

    // читаем файл построчно: КЛЮЧ=значение
    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;  // пустые строки и комментарии пропускаем
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        values[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }

    // достаёт значение по ключу, а если ключа нет - сообщает об ошибке
    auto get = [&](const std::string& key) -> std::string {
        if (values.count(key) == 0) {
            throw std::runtime_error("missing key in .env: " + key);
        }
        return values[key];
    };

    Config cfg;
    cfg.db_host = get("DB_HOST");
    cfg.db_port = get("DB_PORT");
    cfg.db_name = get("DB_NAME");
    cfg.test_db_name = get("TEST_DB_NAME");
    cfg.db_user = get("DB_USER");
    cfg.db_password = get("DB_PASSWORD");
    cfg.server_port = std::stoi(get("SERVER_PORT"));
    cfg.migrations_dir = std::string(PROJECT_DIR) + "/" + get("MIGRATIONS_DIR");
    return cfg;
}

std::string make_conn_string(const Config& cfg, const std::string& db_name) {
    return "host=" + cfg.db_host + " port=" + cfg.db_port + " dbname=" + db_name +
           " user=" + cfg.db_user + " password=" + cfg.db_password;
}
