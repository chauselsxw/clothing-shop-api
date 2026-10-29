#pragma once
#include <string>

// Все настройки приложения, прочитанные из файла .env
struct Config {
    std::string db_host;
    std::string db_port;
    std::string db_name;
    std::string test_db_name;
    std::string db_user;
    std::string db_password;
    int server_port = 8080;
    std::string migrations_dir;
};

// Читает файл .env из корня проекта
Config load_config();

// Собирает строку подключения для libpqxx
std::string make_conn_string(const Config& cfg, const std::string& db_name);
