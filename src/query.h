#pragma once
#include <string>

#include <pqxx/pqxx>

// Выполняет SQL-запрос с параметрами $1, $2, ... (так значения подставляются безопасно,
// без склейки строк - это защита от SQL-инъекций).
// В libpqxx 7 для этого есть функция exec_params, в libpqxx 8 её заменили на
// exec(запрос, params). Эта обёртка нужна, чтобы код собирался с обеими версиями.
template <typename... Args>
pqxx::result query(pqxx::work& tx, const std::string& sql, const Args&... args) {
#if PQXX_VERSION_MAJOR >= 8
    return tx.exec(sql, pqxx::params{args...});
#else
    return tx.exec_params(sql, args...);
#endif
}
