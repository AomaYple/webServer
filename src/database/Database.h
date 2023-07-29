#pragma once

#include <mysql/mysql.h>

#include <string_view>
#include <vector>

class Database {
public:
    Database(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
             unsigned int port, std::string_view unixSocket, unsigned long clientFlag);

    Database(const Database &) = delete;

    ~Database();

private:
    [[nodiscard]] static auto initialize() -> MYSQL *;

    auto connect(std::string_view host, std::string_view user, std::string_view password, std::string_view database,
                 unsigned int port, std::string_view unixSocket, unsigned long clientFlag) -> void;

    auto query(std::string_view statement) -> void;

    [[nodiscard]] auto storeResult() -> MYSQL_RES *;

    [[nodiscard]] static auto getColumnCount(MYSQL_RES *result) -> unsigned int;

    [[nodiscard]] static auto getRow(MYSQL_RES *result) -> MYSQL_ROW;

    static auto freeResult(MYSQL_RES *result) -> void;

    MYSQL *connection;
};