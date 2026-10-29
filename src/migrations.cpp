#include "migrations.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <pqxx/pqxx>

#include "query.h"

void run_migrations(const std::string& conn_string, const std::string& dir) {
    pqxx::connection conn(conn_string);

    // Служебная таблица: в ней запоминаем, какие миграции уже применены
    {
        pqxx::work tx(conn);
        tx.exec(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "filename VARCHAR(200) PRIMARY KEY, "
            "applied_at TIMESTAMP NOT NULL DEFAULT now())");
        tx.commit();
    }

    // Собираем все .sql файлы из папки и сортируем по имени (001, 002, ...)
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".sql") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());

    for (const auto& file : files) {
        std::string name = file.filename().string();

        pqxx::work tx(conn);
        pqxx::result done = query(tx, "SELECT 1 FROM schema_migrations WHERE filename = $1", name);
        if (!done.empty()) {
            continue;  // эта миграция уже была применена раньше
        }

        // читаем весь файл в строку
        std::ifstream in(file);
        std::stringstream buffer;
        buffer << in.rdbuf();

        // выполняем SQL и записываем имя файла в служебную таблицу
        tx.exec(buffer.str());
        query(tx, "INSERT INTO schema_migrations (filename) VALUES ($1)", name);
        tx.commit();
        std::cout << "applied migration: " << name << "\n";
    }
}
