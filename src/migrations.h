#pragma once
#include <string>

// Применяет к базе все ещё не применённые .sql файлы из папки dir
// (в порядке возрастания имён файлов).
void run_migrations(const std::string& conn_string, const std::string& dir);
